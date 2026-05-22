// Copyright (c) 2026 The BlakeBitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <auxpow.h>
#include <chain.h>
#include <chainparams.h>
#include <clientversion.h>
#include <consensus/merkle.h>
#include <pow.h>
#include <primitives/block.h>
#include <streams.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <functional>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(auxpow_tests, BasicTestingSetup)

static Consensus::Params AuxpowTestParams()
{
    auto chain_params = CreateChainParams(ArgsManager{}, CBaseChainParams::REGTEST);
    Consensus::Params params = chain_params->GetConsensus();
    params.fStrictChainId = true;
    params.nAuxpowChainId = 0x0005;
    params.nAuxpowStartHeight = 0;
    return params;
}

static unsigned int PowLimitBits(const Consensus::Params& params)
{
    return UintToArith256(params.powLimit).GetCompact();
}

static void MineNativeHeader(CPureBlockHeader& header, const Consensus::Params& params)
{
    while (!CheckProofOfWork(header.GetPoWHash(), header.nBits, params)) {
        ++header.nNonce;
    }
}

static CBlockHeader NativeHeader(const Consensus::Params& params)
{
    CBlockHeader header;
    header.nVersion = CPureBlockHeader::BLOCK_VERSION_DEFAULT;
    header.nBits = PowLimitBits(params);
    MineNativeHeader(header, params);
    return header;
}

static CBlockHeader AuxpowHeader(const Consensus::Params& params, int chain_id)
{
    CBlockHeader header;
    header.nVersion = CPureBlockHeader::BLOCK_VERSION_DEFAULT;
    header.SetChainId(chain_id);
    header.nBits = PowLimitBits(params);
    CAuxPow::InitAuxPow(header);

    while (!CheckProofOfWork(header.auxpow->GetParentBlockPoWHash(), header.nBits, params)) {
        ++header.auxpow->parentBlock.nNonce;
    }

    return header;
}

static void AppendLE32(std::vector<unsigned char>& data, uint32_t value)
{
    for (unsigned int i = 0; i < 4; ++i) {
        data.push_back((value >> (8 * i)) & 0xff);
    }
}

static std::vector<unsigned char> ReversedHashBytes(const uint256& hash)
{
    std::vector<unsigned char> bytes(hash.begin(), hash.end());
    std::reverse(bytes.begin(), bytes.end());
    return bytes;
}

static CScript RawScript(const std::vector<unsigned char>& data)
{
    CScript script;
    script.insert(script.end(), data.begin(), data.end());
    return script;
}

static std::vector<unsigned char> ChainCommitment(const uint256& child_hash, uint32_t merkle_size = 1, uint32_t nonce = 0)
{
    std::vector<unsigned char> data = ReversedHashBytes(child_hash);
    AppendLE32(data, merkle_size);
    AppendLE32(data, nonce);
    return data;
}

static CBlockHeader AuxpowHeaderWithCoinbaseScript(
    const Consensus::Params& params,
    const std::function<std::vector<unsigned char>(const uint256&)>& script_data_fn,
    std::vector<uint256> chain_merkle_branch = {},
    unsigned int chain_index = 0)
{
    CBlockHeader header;
    header.nVersion = CPureBlockHeader::BLOCK_VERSION_DEFAULT;
    header.SetChainId(params.nAuxpowChainId);
    header.nBits = PowLimitBits(params);
    header.SetAuxpowFlag(true);

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vin[0].scriptSig = RawScript(script_data_fn(header.GetHash()));
    CTransactionRef coinbase_ref = MakeTransactionRef(std::move(coinbase));

    CBlock parent;
    parent.nVersion = 1;
    parent.vtx.resize(1);
    parent.vtx[0] = coinbase_ref;
    parent.hashMerkleRoot = BlockMerkleRoot(parent);

    header.SetAuxpow(new CAuxPow(coinbase_ref));
    header.auxpow->vChainMerkleBranch = std::move(chain_merkle_branch);
    header.auxpow->nChainIndex = chain_index;
    header.auxpow->parentBlock = parent;

    while (!CheckProofOfWork(header.auxpow->GetParentBlockPoWHash(), header.nBits, params)) {
        ++header.auxpow->parentBlock.nNonce;
    }

    return header;
}

BOOST_AUTO_TEST_CASE(header_serialization_without_auxpow)
{
    const Consensus::Params params = AuxpowTestParams();
    const CBlockHeader header = NativeHeader(params);

    DataStream stream{};
    stream << header;
    BOOST_CHECK_EQUAL(stream.size(), 80U);

    CBlockHeader roundtrip;
    stream >> roundtrip;
    BOOST_CHECK(!roundtrip.auxpow);
    BOOST_CHECK_EQUAL(roundtrip.GetHash().ToString(), header.GetHash().ToString());
}

BOOST_AUTO_TEST_CASE(header_serialization_with_auxpow)
{
    const Consensus::Params params = AuxpowTestParams();
    const CBlockHeader header = AuxpowHeader(params, params.nAuxpowChainId);

    DataStream stream{};
    stream << header;
    BOOST_CHECK_GT(stream.size(), 80U);

    CBlockHeader roundtrip;
    stream >> roundtrip;
    BOOST_REQUIRE(roundtrip.auxpow);
    BOOST_CHECK(roundtrip.IsAuxpow());
    BOOST_CHECK_EQUAL(roundtrip.GetHash().ToString(), header.GetHash().ToString());
}

BOOST_AUTO_TEST_CASE(auxpow_payload_does_not_change_child_hash)
{
    const Consensus::Params params = AuxpowTestParams();
    CBlockHeader header = AuxpowHeader(params, params.nAuxpowChainId);
    const uint256 child_hash = header.GetHash();

    ++header.auxpow->parentBlock.nNonce;
    BOOST_CHECK_EQUAL(header.GetHash().ToString(), child_hash.ToString());
}

BOOST_AUTO_TEST_CASE(disk_block_index_persists_auxpow_payload)
{
    const Consensus::Params params = AuxpowTestParams();
    const CBlockHeader header = AuxpowHeader(params, params.nAuxpowChainId);
    const uint256 hash = header.GetHash();

    CBlockIndex index(header);
    index.phashBlock = &hash;

    CDiskBlockIndex disk_index(&index);
    CDataStream stream{SER_DISK, CLIENT_VERSION};
    stream << disk_index;

    CDiskBlockIndex roundtrip;
    stream >> roundtrip;
    BOOST_REQUIRE(roundtrip.auxpow);

    const CBlockHeader restored_header = roundtrip.GetBlockHeader();
    BOOST_CHECK(restored_header.IsAuxpow());
    BOOST_CHECK_EQUAL(restored_header.GetHash().ToString(), header.GetHash().ToString());

    DataStream header_stream{};
    header_stream << restored_header;
    BOOST_CHECK_GT(header_stream.size(), 80U);
}

BOOST_AUTO_TEST_CASE(disk_block_index_rejects_missing_auxpow_payload)
{
    const Consensus::Params params = AuxpowTestParams();
    CBlockHeader header = NativeHeader(params);
    header.SetAuxpowFlag(true);
    const uint256 hash = header.GetHash();

    CBlockIndex index(header);
    index.phashBlock = &hash;

    CDiskBlockIndex disk_index(&index);
    CDataStream stream{SER_DISK, CLIENT_VERSION};
    BOOST_CHECK_THROW(stream << disk_index, std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(auxpow_wrong_chain_id_rejected)
{
    const Consensus::Params params = AuxpowTestParams();
    const CBlockHeader header = AuxpowHeader(params, params.nAuxpowChainId + 1);

    BOOST_CHECK(!CheckAuxPowProofOfWork(header, params, params.nAuxpowStartHeight));
}

BOOST_AUTO_TEST_CASE(auxpow_pre_activation_payload_tolerated)
{
    Consensus::Params params = AuxpowTestParams();
    params.nAuxpowStartHeight = 100;

    CBlockHeader header = AuxpowHeader(params, params.nAuxpowChainId + 1);
    header.auxpow->parentBlock.nNonce = 0;

    BOOST_CHECK(CheckAuxPowProofOfWork(header, params, 99));
}

BOOST_AUTO_TEST_CASE(auxpow_active_valid)
{
    const Consensus::Params params = AuxpowTestParams();
    const CBlockHeader header = AuxpowHeader(params, params.nAuxpowChainId);

    BOOST_CHECK(CheckAuxPowProofOfWork(header, params, params.nAuxpowStartHeight));
}

BOOST_AUTO_TEST_CASE(auxpow_missing_parent_coinbase_rejected)
{
    const Consensus::Params params = AuxpowTestParams();
    CBlockHeader header;
    header.nVersion = CPureBlockHeader::BLOCK_VERSION_DEFAULT;
    header.SetChainId(params.nAuxpowChainId);
    header.nBits = PowLimitBits(params);
    header.SetAuxpow(new CAuxPow());
    header.auxpow->parentBlock.nVersion = 1;

    while (!CheckProofOfWork(header.auxpow->GetParentBlockPoWHash(), header.nBits, params)) {
        ++header.auxpow->parentBlock.nNonce;
    }

    BOOST_CHECK(!CheckAuxPowProofOfWork(header, params, params.nAuxpowStartHeight));
}

BOOST_AUTO_TEST_CASE(auxpow_missing_chain_merkle_root_rejected)
{
    const Consensus::Params params = AuxpowTestParams();
    const CBlockHeader header = AuxpowHeaderWithCoinbaseScript(params, [](const uint256&) {
        return std::vector<unsigned char>{'n', 'o', '-', 'r', 'o', 'o', 't'};
    });

    BOOST_CHECK(!CheckAuxPowProofOfWork(header, params, params.nAuxpowStartHeight));
}

BOOST_AUTO_TEST_CASE(auxpow_multiple_merged_mining_headers_rejected)
{
    const Consensus::Params params = AuxpowTestParams();
    const CBlockHeader header = AuxpowHeaderWithCoinbaseScript(params, [](const uint256& child_hash) {
        std::vector<unsigned char> data(std::begin(PCH_MERGED_MINING_HEADER), std::end(PCH_MERGED_MINING_HEADER));
        data.insert(data.end(), std::begin(PCH_MERGED_MINING_HEADER), std::end(PCH_MERGED_MINING_HEADER));
        const std::vector<unsigned char> commitment = ChainCommitment(child_hash);
        data.insert(data.end(), commitment.begin(), commitment.end());
        return data;
    });

    BOOST_CHECK(!CheckAuxPowProofOfWork(header, params, params.nAuxpowStartHeight));
}

BOOST_AUTO_TEST_CASE(auxpow_root_after_first_twenty_bytes_without_magic_rejected)
{
    const Consensus::Params params = AuxpowTestParams();
    const CBlockHeader header = AuxpowHeaderWithCoinbaseScript(params, [](const uint256& child_hash) {
        std::vector<unsigned char> data(21, 0);
        const std::vector<unsigned char> commitment = ChainCommitment(child_hash);
        data.insert(data.end(), commitment.begin(), commitment.end());
        return data;
    });

    BOOST_CHECK(!CheckAuxPowProofOfWork(header, params, params.nAuxpowStartHeight));
}

BOOST_AUTO_TEST_CASE(auxpow_merkle_size_mismatch_rejected)
{
    const Consensus::Params params = AuxpowTestParams();
    const CBlockHeader header = AuxpowHeaderWithCoinbaseScript(params, [](const uint256& child_hash) {
        return ChainCommitment(child_hash, /*merkle_size=*/2, /*nonce=*/0);
    });

    BOOST_CHECK(!CheckAuxPowProofOfWork(header, params, params.nAuxpowStartHeight));
}

BOOST_AUTO_TEST_CASE(auxpow_wrong_chain_index_rejected)
{
    const Consensus::Params params = AuxpowTestParams();
    const CBlockHeader header = AuxpowHeaderWithCoinbaseScript(params, [](const uint256& child_hash) {
        return ChainCommitment(child_hash);
    }, /*chain_merkle_branch=*/{}, /*chain_index=*/1);

    BOOST_CHECK(!CheckAuxPowProofOfWork(header, params, params.nAuxpowStartHeight));
}

BOOST_AUTO_TEST_CASE(auxpow_chain_merkle_branch_too_long_rejected)
{
    const Consensus::Params params = AuxpowTestParams();
    std::vector<uint256> branch(31);
    const CBlockHeader header = AuxpowHeaderWithCoinbaseScript(params, [](const uint256& child_hash) {
        return ChainCommitment(child_hash);
    }, std::move(branch));

    BOOST_CHECK(!CheckAuxPowProofOfWork(header, params, params.nAuxpowStartHeight));
}

BOOST_AUTO_TEST_SUITE_END()
