// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2013-2026 The BlakeBitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>
#include <limits>

#include "chainparamsseeds.h"

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * BlakeBitcoin Genesis Block:
 * CBlock(hash=0000000f14c5..., ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1372066561, nBits=1d00ffff, nNonce=421575, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d010445...)
 *     CTxOut(nValue=50.00000000, scriptPubKey=...)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    // BlakeBitcoin legacy genesis block parameters.
    const char* pszTimestamp = "Added to the Blake-256 Merge Mining 12th May 2014";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        // BlakeBitcoin keeps the historical version checks disabled in this bootstrap.
        consensus.BIP34Height = 100000000; // Disabled - far in future
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 100000000; // Disabled - far in future
        consensus.BIP66Height = 100000000; // Disabled - far in future
        consensus.powLimit = uint256S("0x000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 150;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.fStrictChainId = true;
        consensus.nAuxpowChainId = 0x0005;
        consensus.nAuxpowStartHeight = 500000;
        consensus.nRuleChangeActivationThreshold = 6048; // 75% of 8064
        consensus.nMinerConfirmationWindow = 8064;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // Mainnet SegWit signaling starts on May 11, 2026 00:00:00 UTC and
        // times out on May 11, 2027 00:00:00 UTC.
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1778457600;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1809993600;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbc;
        pchMessageStart[2] = 0xa7;
        pchMessageStart[3] = 0xb7;
        nDefaultPort = 8356;
        nPruneAfterHeight = 100000;

        // BlakeBitcoin genesis block.
        // pszTimestamp: "Added to the Blake-256 Merge Mining 12th May 2014"
        genesis = CreateGenesisBlock(1399109785, 183893667, 503382015, 112, 50 * COIN);
        consensus.hashGenesisBlock = uint256S("0x000000dcb4434e2148558a0a5c71e5c06d864accef97d75ac1c031405deb3371");
        assert(genesis.hashMerkleRoot == uint256S("0x0423141660220f9f155a4129a49dcb6431bbed9cd037bba3da34c2baa53ed0ac"));

        // BlakeStream ecosystem DNS seeds — shared across all 6 coins
        vSeeds.emplace_back("blakestream.io", "seed.blakestream.io", false);
        vSeeds.emplace_back("blakecoin.org", "seed.blakecoin.org", false);

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,243);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,7);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
        bech32_hrp = "bbtc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        // Keep mainnet non-mineable here until that work is ported and QC'd.
        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = (CCheckpointData) {
            {
                {0,       uint256S("0x000000dcb4434e2148558a0a5c71e5c06d864accef97d75ac1c031405deb3371")},
                {145025,  uint256S("0x0316c10a202c2bde44628c8cac2d75d61f078a1d961ae1499eaa98eb643b5068")},
                {179266,  uint256S("0x7b102e1f37971dcd4311cc64f83fc62da0f75c22270e831be0a6c8cc38ddd5c8")},
                {338643,  uint256S("0xdd79a4b1ac2a91d9666d97a2654ee826c84e55495665bacaac2b9a953616f8d6")},
                {406644,  uint256S("0xc0f29fe22936216e6a90a4178967ba8ffa9ad78930aa1a369a6fc727a3d2f8e5")},
                {845000,  uint256S("0xa32d61133e22687a63c0c2769552a851a484b030cda02f8a1def5a506d368e33")},
                {900000,  uint256S("0x1054253e8fd7b596cdddd562619da01022024623ee72ed3b37ea909c3caa5cc7")},
                {950000,  uint256S("0xdd4b0d5c8ae8dca9b0fd8ebee3fdf1312c47cdf73f18bff2379d5e4e2d1e59c8")},
                {966300,  uint256S("0xb4d0943e70a43256e5a329e7f7450abc42cd1aa9a8f277b0ef7a990dccbbe800")},
                {1004800, uint256S("0xea2e486ef9e96a02e8bfa6782cb2aa36b393783af0b4793dcaa58747f70cd71d")},
                {1352360, uint256S("0xadf9abb289d6e69ca373b4c6dc5853a7c444ea6db5bc1e33b6df2c061eb9444a")},
                {1719100, uint256S("0xe1572e3497370fc796c9722f28c570db392d284f574eb652cb953e39b14a0127")},
            }
        };

        chainTxData = ChainTxData{
            1646509845,
            2132261,
            60000.0
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 100000000; // Disabled - far in future
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 100000000; // Disabled - far in future
        consensus.BIP66Height = 100000000; // Disabled - far in future
        consensus.powLimit = uint256S("0x000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 150;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.fStrictChainId = false;
        consensus.nAuxpowChainId = 0x0005;
        consensus.nAuxpowStartHeight = 0;
        consensus.nRuleChangeActivationThreshold = 6048;
        consensus.nMinerConfirmationWindow = 8064;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x12;
        pchMessageStart[3] = 0x09;
        nDefaultPort = 18112;
        nPruneAfterHeight = 1000;

        // Legacy 0.8 BlakeBitcoin testnet genesis (from main.cpp:2869-2870 + 2820).
        // Same coinbase tx as mainnet (so merkle root matches), distinct nTime + nNonce.
        // Genesis PoW check is bypassed in validation.cpp when block hash equals
        // consensus.hashGenesisBlock, so 0x0000ffff... target is not enforced here.
        genesis = CreateGenesisBlock(1392351202, 4335147, 503382015, 112, 50 * COIN);
        consensus.hashGenesisBlock = uint256S("0x00000052d978f26d698e0c4dbce9f8139a69f2fbda37715149146776aeb70d5b");
        assert(genesis.hashMerkleRoot == uint256S("0x0423141660220f9f155a4129a49dcb6431bbed9cd037bba3da34c2baa53ed0ac"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // Testnet seeds are intentionally omitted in this bootstrap.

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,142);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,170);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};
        bech32_hrp = "tbbtc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;

        checkpointData = (CCheckpointData) {
            {
                {0, uint256S("0x0000007381461a5b95ec93210ec6fcf6d3328e7f34113da11a4657d06caef7ad")},
            }
        };

        chainTxData = ChainTxData{
            1392351202,
            0,
            60000.0
        };

    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 100000000; // BIP34 remains disabled in this local-only harness
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // Keep regtest local-only and easy to mine, but align the cadence with BlakeBitcoin.
        consensus.nPowTargetTimespan = 150 * 144;
        consensus.nPowTargetSpacing = 150;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.fStrictChainId = false;
        consensus.nAuxpowChainId = 0x0005;
        consensus.nAuxpowStartHeight = 0;
        consensus.nRuleChangeActivationThreshold = 108;
        consensus.nMinerConfirmationWindow = 144;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;

        // Regtest intentionally reuses the mainnet genesis until a dedicated local genesis is defined.
        genesis = CreateGenesisBlock(1399109785, 183893667, 503382015, 112, 50 * COIN);
        consensus.hashGenesisBlock = uint256S("0x000000dcb4434e2148558a0a5c71e5c06d864accef97d75ac1c031405deb3371");
        assert(genesis.hashMerkleRoot == uint256S("0x0423141660220f9f155a4129a49dcb6431bbed9cd037bba3da34c2baa53ed0ac"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = (CCheckpointData){
            {
                {0, uint256S("0x000000dcb4434e2148558a0a5c71e5c06d864accef97d75ac1c031405deb3371")}
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        // Regtest stays local-only, but we reuse the mainnet-style Base58 prefixes for now.
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,243);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,7);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
        bech32_hrp = "rbbtc";
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
