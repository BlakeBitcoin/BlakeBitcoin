// Copyright (c) 2011 Vince Durham
// Copyright (c) 2009-present The Bitcoin Core developers
// Copyright (c) 2013-2026 The BlakeBitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <auxpow.h>

#include <compat/endian.h>
#include <consensus/merkle.h>
#include <hash.h>
#include <logging.h>
#include <primitives/block.h>
#include <script/script.h>

#include <algorithm>
#include <cstring>

bool CAuxPow::Check(const uint256& hash_aux_block, int chain_id, const Consensus::Params& params) const
{
    if (params.fStrictChainId && parentBlock.GetChainId() == chain_id) {
        return error("Aux POW parent has our chain ID");
    }

    if (vChainMerkleBranch.size() > 30) {
        return error("Aux POW chain merkle branch too long");
    }

    const uint256 root_hash = CheckMerkleBranch(hash_aux_block, vChainMerkleBranch, nChainIndex);
    std::vector<unsigned char> root_bytes(root_hash.begin(), root_hash.end());
    std::reverse(root_bytes.begin(), root_bytes.end());

    if (!m_coinbase_tx) {
        return error("Aux POW missing parent coinbase transaction");
    }

    if (CheckMerkleBranch(m_coinbase_tx->GetHash(), m_merkle_branch, 0) != parentBlock.hashMerkleRoot) {
        return error("Aux POW merkle root incorrect");
    }

    if (m_coinbase_tx->vin.empty()) {
        return error("Aux POW coinbase has no inputs");
    }

    const CScript& script = m_coinbase_tx->vin[0].scriptSig;
    auto header_pos = std::search(script.begin(), script.end(), std::begin(PCH_MERGED_MINING_HEADER), std::end(PCH_MERGED_MINING_HEADER));
    auto root_pos = std::search(script.begin(), script.end(), root_bytes.begin(), root_bytes.end());

    if (root_pos == script.end()) {
        return error("Aux POW missing chain merkle root in parent coinbase");
    }

    if (header_pos != script.end()) {
        if (script.end() != std::search(header_pos + 1, script.end(), std::begin(PCH_MERGED_MINING_HEADER), std::end(PCH_MERGED_MINING_HEADER))) {
            return error("Multiple merged mining headers in coinbase");
        }
        if (header_pos + sizeof(PCH_MERGED_MINING_HEADER) != root_pos) {
            return error("Merged mining header is not just before chain merkle root");
        }
    } else if (root_pos - script.begin() > 20) {
        return error("Aux POW chain merkle root must start in the first 20 bytes of the parent coinbase");
    }

    root_pos += root_bytes.size();
    if (script.end() - root_pos < 8) {
        return error("Aux POW missing chain merkle tree size and nonce in parent coinbase");
    }

    uint32_t merkle_size;
    std::memcpy(&merkle_size, &root_pos[0], sizeof(merkle_size));
    merkle_size = le32toh(merkle_size);

    const unsigned merkle_height = vChainMerkleBranch.size();
    if (merkle_size != (1u << merkle_height)) {
        return error("Aux POW merkle branch size does not match parent coinbase");
    }

    uint32_t nonce;
    std::memcpy(&nonce, &root_pos[4], sizeof(nonce));
    nonce = le32toh(nonce);
    if (nChainIndex != static_cast<unsigned int>(GetExpectedIndex(nonce, chain_id, merkle_height))) {
        return error("Aux POW wrong index");
    }

    return true;
}

int CAuxPow::GetExpectedIndex(uint32_t nonce, int chain_id, unsigned height)
{
    uint32_t rand = nonce;
    rand = rand * 1103515245 + 12345;
    rand += chain_id;
    rand = rand * 1103515245 + 12345;

    return rand % (1u << height);
}

uint256 CAuxPow::CheckMerkleBranch(uint256 hash, const std::vector<uint256>& merkle_branch, int index)
{
    if (index == -1) {
        return uint256{};
    }

    for (const uint256& branch_hash : merkle_branch) {
        if (index & 1) {
            hash = Hash(branch_hash, hash);
        } else {
            hash = Hash(hash, branch_hash);
        }
        index >>= 1;
    }

    return hash;
}

void CAuxPow::InitAuxPow(CBlockHeader& header)
{
    header.SetAuxpowFlag(true);

    const uint256 block_hash = header.GetHash();
    std::vector<unsigned char> input_data(block_hash.begin(), block_hash.end());
    std::reverse(input_data.begin(), input_data.end());
    input_data.push_back(1);
    input_data.insert(input_data.end(), 7, 0);

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vin[0].scriptSig = CScript{} << input_data;
    CTransactionRef coinbase_ref = MakeTransactionRef(std::move(coinbase));

    CBlock parent;
    parent.nVersion = 1;
    parent.vtx.resize(1);
    parent.vtx[0] = coinbase_ref;
    parent.hashMerkleRoot = BlockMerkleRoot(parent);

    header.SetAuxpow(new CAuxPow(coinbase_ref));
    header.auxpow->nChainIndex = 0;
    header.auxpow->parentBlock = parent;
}
