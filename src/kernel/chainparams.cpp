// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <hash.h>
#include <chainparamsbase.h>
#include <logging.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

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
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock CreateBlakeBitcoinGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    // BlakeBitcoin's own coinbase scriptSig text from coin-source-of-truth.md.
    // Distinct genesis hash from the rest of the Blakestream family.
    const char* pszTimestamp = "Added to the Blake-256 Merge Mining 12th May 2014";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock CreatePhotonSharedTestnetGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    // BBTC testnet/signet intentionally preserve the legacy 0.8/0.15.21/SOT
    // testnet hash by sharing Photon's byte-identical testnet genesis block.
    // Mainnet and regtest use BlakeBitcoin's own genesis block.
    const char* pszTimestamp = "US forces target leading al-Shabaab militant in Somalian coastal raid";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network on which people trade goods and services.
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        // BlakeBitcoin uses BBTC halving (50 BBTC initial,
        // halve every 210000 blocks) — distinct from the flat-subsidy parent
        // and Photon (perpetual difficulty-aware reward). See
        // coin-source-of-truth.md "BlakeBitcoin (BBTC)" Subsidy line.
        consensus.nSubsidyHalvingInterval = 210000;
        // BBTC 0.25.2 buries the recorded 0.15.21 SegWit ACTIVE height and
        // schedules the post-rebase cleanup BIPs in the later family window.
        consensus.BIP34Height = 2572228;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 2572228;
        consensus.BIP66Height = 2572228;
        // TODO(blakestream-25.2-activation): CSV (BIP68/112/113) — atomic-swap timeout
        // primitive. ALWAYS_ACTIVE from genesis on Blakestream family per
        // coin-source-of-truth.md "Common rules". Do NOT change.
        consensus.CSVHeight = 1;
        // SegWit (BIP141/143/147) — atomic-swap anchor (P2WSH HTLC). BBTC
        // 0.25.2 inherits the 0.15.21 BIP9 activation result as a buried height
        // and does not create a second SegWit signaling window.
        consensus.SegwitHeight = 2564352;
        consensus.MinBIP9WarningHeight = 0;
        // BBTC-0.15.21 powLimit (from src/chainparams.cpp:87) is the wider mask
        // ~uint256(0)>>24 = 0x000000FF...FF (3 bytes zero, 29 bytes ff). NOT the
        // narrower 0x000000FFFF000... used elsewhere in the family. This is the
        // BBTC legacy "loose powLimit + tight retarget" pattern. The
        // coin-source-of-truth.md BBTC entry calls this "compact 0x1e00ffff" but
        // those decode to different 256-bit values; the BBTC-0.15.21 source is
        // canonical.
        consensus.powLimit = uint256S("0x000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // BBTC uses a 14-day retarget at 2.5-min spacing (per
        // BBTC-0.15.21 src/chainparams.cpp:88-89 and coin-source-of-truth.md).
        // Distinct from the 1-hour 20-block retarget at 3-min
        // spacing.
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 150;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        // BBTC BIP9 deployment threshold/window match Bitcoin's 75%/8064 — wider
        // than the 95%/20 family window because BBTC's 14-day retarget gives
        // miners much longer windows to organise.
        consensus.nRuleChangeActivationThreshold = 6048;
        consensus.nMinerConfirmationWindow = 8064;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0;
        // Taproot follows the shared aux-coin 0.25.2 window; BBTC uses its own
        // 150-second spacing to derive the minimum activation height.
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = 1782871200;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = 1814407200;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 2576260;

        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000007b0e2d1ac7cf3ffec6");
        consensus.defaultAssumeValid = uint256S("0x8a932c074fe7d928093e3d75dacdc6e60f5895adeb339d9d1af3972a3932bd2e");

        // BlakeBitcoin AuxPoW chain identity (consumed by Phase 2 AuxPoW core).
        // mainnet: strict chain-ID, AuxPoW activates at historical height 500000.
        // Per coin-source-of-truth.md > AuxPoW chain ID registry: BBTC = 0x0005
        // (Photon = 0x0002, Lithium = 0x0006, ELT = 0x0007, UMO = 0x000F).
        consensus.fStrictChainId = true;
        consensus.nAuxpowChainId = 0x0005;
        consensus.nAuxpowStartHeight = 500000;

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         *
         * BlakeBitcoin mainnet magic from coin-source-of-truth.md and BBTC-0.15.21
         * src/chainparams.cpp:123-126. Distinct from the parent (0xf9 0xbe 0xb4 0xd2)
         * and Photon (0xf9 0xbc 0xb4 0xd2).
         */
        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbc;
        pchMessageStart[2] = 0xa7;
        pchMessageStart[3] = 0xb7;
        nDefaultPort = 8356;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 6;
        m_assumed_chain_state_size = 1;

        // BlakeBitcoin mainnet genesis from coin-source-of-truth.md and BBTC-0.15.21
        // src/chainparams.cpp:132-134. nTime = 1399109785 (12 May 2014), nNonce
        // 183893667, nBits 0x1e00ffff, nVersion 112, reward 50 BBTC.
        genesis = CreateBlakeBitcoinGenesisBlock(1399109785, 183893667, 503382015, 112, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000dcb4434e2148558a0a5c71e5c06d864accef97d75ac1c031405deb3371"));
        assert(genesis.hashMerkleRoot == uint256S("0x0423141660220f9f155a4129a49dcb6431bbed9cd037bba3da34c2baa53ed0ac"));

        // Shared BlakeStream family DNS seeds.
        vSeeds.emplace_back("seed.blakestream.io");
        vSeeds.emplace_back("seed.blakecoin.org");

        // BlakeBitcoin address prefixes — mainnet pubkey 243 (addresses begin
        // with the prefix encoded into the leading character set; distinct from
        // parent/family value 26). See coin-source-of-truth.md.
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,243);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,7);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "bbtc";
        vFixedSeeds.clear();

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        // BlakeBitcoin mainnet checkpoints from BlakeBitcoin-0.15.21 and
        // coin-source-of-truth.md.
        checkpointData = {
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
                {2528298, uint256S("0x8a932c074fe7d928093e3d75dacdc6e60f5895adeb339d9d1af3972a3932bd2e")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
            // Intentionally empty for BlakeBitcoin mainnet.
            //
            // Snapshot RPC infrastructure may be present, but do not hardcode or
            // publish mainnet assumeutxo metadata until BlakeBitcoin's SegWit
            // activation state and snapshot policy are finalized. With no entry
            // here, mainnet loadtxoutset rejects snapshots instead of accepting
            // unapproved checkpoint metadata.
        };

        // ChainTxData is used by GuessVerificationProgress for operator-facing
        // sync/readiness estimates. It is not consensus data. dTxRate must stay
        // in transactions per second; a per-day value here makes a healthy node
        // report low verification progress even when blocks and headers match.
        chainTxData = ChainTxData{
            .nTime    = 1775981524,
            .nTxCount = 2949114,
            .dTxRate  = 0.007159151156625834,
        };
    }
};

/**
 * Testnet (v3): public test network which is reset from time to time.
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        // BlakeBitcoin testnet matches mainnet halving (210000).
        consensus.nSubsidyHalvingInterval = 210000;
        // Testnet is a 0.25.2 feature-test network.
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        // TODO(blakestream-25.2-activation): CSV (BIP68/112/113) ALWAYS_ACTIVE on
        // Blakestream family — atomic-swap timeout primitive. Do NOT change.
        consensus.CSVHeight = 1;
        // TODO(blakestream-25.2-activation): testnet SegWit ALWAYS_ACTIVE from height
        // 1 so testnet AuxPoW + atomic-swap regression coverage works without
        // waiting on the 0.15.21 mainnet activation. Do NOT change.
        consensus.SegwitHeight = 1;
        consensus.MinBIP9WarningHeight = 0;
        // Private 25.2 feature-testnet uses regtest-style PoW so local CPU
        // pool tests can advance blocks quickly while mainnet stays unchanged.
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 150;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 6048;
        consensus.nMinerConfirmationWindow = 8064;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0;

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        // BlakeBitcoin AuxPoW: testnet does NOT enforce strict chain-ID, and AuxPoW
        // is acceptable from genesis (no historical pre-AuxPoW height).
        consensus.fStrictChainId = false;
        consensus.nAuxpowChainId = 0x0005;
        consensus.nAuxpowStartHeight = 0;

        // BlakeBitcoin testnet magic from coin-source-of-truth.md and
        // BBTC-0.15.21 src/chainparams.cpp:222-225.
        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x12;
        pchMessageStart[3] = 0x09;
        nDefaultPort = 18112;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        // BBTC testnet keeps the legacy 0.8/0.15.21/SOT hash by sharing
        // Photon's testnet genesis block byte-for-byte. This is testnet-only;
        // mainnet keeps BlakeBitcoin's own genesis block and 50 BBTC reward.
        genesis = CreatePhotonSharedTestnetGenesisBlock(1392351202, 4335147, 503382015, 112, 32768 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x00000052d978f26d698e0c4dbce9f8139a69f2fbda37715149146776aeb70d5b"));
        assert(genesis.hashMerkleRoot == uint256S("0x251e462b7d8b2e92e74651186fbbc66ac715cf9c160212efb02642232207112d"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // BlakeBitcoin testnet seeds to be added when available.

        // BBTC testnet base58 prefixes match the Blakestream family testnet
        // standard (142/170/239) per coin-source-of-truth.md "Common rules".
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,142);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,170);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tbbtc";

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                {0, uint256S("0x00000052d978f26d698e0c4dbce9f8139a69f2fbda37715149146776aeb70d5b")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
            // TODO to be specified in a future patch.
        };

        // This estimate feeds verificationprogress only. Keep dTxRate in
        // transactions per second; 60000.0 means 60000 tx/sec, not 60000 tx/day.
        chainTxData = ChainTxData{
            .nTime    = 1392351202,
            .nTxCount = 0,
            .dTxRate  = 60000.0 / (24 * 60 * 60),
        };
    }
};

/**
 * Signet: test network with an additional consensus parameter (see BIP325).
 */
class SigNetParams : public CChainParams {
public:
    explicit SigNetParams(const SigNetOptions& options)
    {
        std::vector<uint8_t> bin;
        vSeeds.clear();

        if (!options.challenge) {
            // BlakeBitcoin signet defaults to a local/private developer network.
            // Keep the default challenge trivial and ship no global seeds,
            // assumevalid, or chainwork so we do not point at Bitcoin signet.
            bin = ParseHex("51");
            consensus.nMinimumChainWork = uint256{};
            consensus.defaultAssumeValid = uint256{};
            m_assumed_blockchain_size = 0;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{0, 0, 0};
        } else {
            bin = *options.challenge;
            consensus.nMinimumChainWork = uint256{};
            consensus.defaultAssumeValid = uint256{};
            m_assumed_blockchain_size = 0;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                0,
                0,
                0,
            };
            LogPrintf("Signet with challenge %s\n", HexStr(bin));
        }

        if (options.seeds) {
            vSeeds = *options.seeds;
        }

        strNetworkID = CBaseChainParams::SIGNET;
        consensus.signet_blocks = true;
        consensus.signet_challenge.assign(bin.begin(), bin.end());
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 1;
        // BBTC signet inherits mainnet consensus (14-day
        // retarget, 2.5-min spacing, 75%/8064 BIP9 window).
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60;
        consensus.nPowTargetSpacing = 150;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 6048;
        consensus.nMinerConfirmationWindow = 8064;
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("00000377ae000000000000000000000000000000000000000000000000000000");
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Keep Taproot always active on signet so developer coverage matches
        // regtest/testnet, while mainnet activation policy waits on
        // BBTC-0.15.21 mainnet SegWit activation results.
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // message start is defined as the first 4 bytes of the sha256d of the block script
        HashWriter h{};
        h << consensus.signet_challenge;
        uint256 hash = h.GetHash();
        memcpy(pchMessageStart, hash.begin(), 4);

        nDefaultPort = 38733;
        nPruneAfterHeight = 1000;

        // BlakeBitcoin signet: defaults to the testnet genesis params (BBTC-0.15.21
        // never shipped mainnet signet; signet is a Bitcoin-Core-25.2 inherited
        // facility used here for developer experimentation only).
        consensus.fStrictChainId = false;
        consensus.nAuxpowChainId = 0x0005;
        consensus.nAuxpowStartHeight = 0;
        genesis = CreatePhotonSharedTestnetGenesisBlock(1392351202, 4335147, 503382015, 112, 32768 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x00000052d978f26d698e0c4dbce9f8139a69f2fbda37715149146776aeb70d5b"));
        assert(genesis.hashMerkleRoot == uint256S("0x251e462b7d8b2e92e74651186fbbc66ac715cf9c160212efb02642232207112d"));

        vFixedSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,142);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,170);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tbbtc";

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = false;
    }
};

/**
 * Regression test: intended for private networks only. Has minimal difficulty to ensure that
 * blocks can be found instantly.
 */
class CRegTestParams : public CChainParams
{
public:
    explicit CRegTestParams(const RegTestOptions& opts)
    {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 100000000;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1351;
        consensus.BIP66Height = 1251;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 150 * 144;
        consensus.nPowTargetSpacing = 150;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108;
        consensus.nMinerConfirmationWindow = 144;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        // BlakeBitcoin regtest: AuxPoW machinery available from genesis, no
        // strict chain-ID enforcement.
        consensus.fStrictChainId = false;
        consensus.nAuxpowChainId = 0x0005;
        consensus.nAuxpowStartHeight = 0;

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = opts.fastprune ? 100 : 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        for (const auto& [dep, height] : opts.activation_heights) {
            switch (dep) {
            case Consensus::BuriedDeployment::DEPLOYMENT_SEGWIT:
                consensus.SegwitHeight = int{height};
                break;
            case Consensus::BuriedDeployment::DEPLOYMENT_HEIGHTINCB:
                consensus.BIP34Height = int{height};
                break;
            case Consensus::BuriedDeployment::DEPLOYMENT_DERSIG:
                consensus.BIP66Height = int{height};
                break;
            case Consensus::BuriedDeployment::DEPLOYMENT_CLTV:
                consensus.BIP65Height = int{height};
                break;
            case Consensus::BuriedDeployment::DEPLOYMENT_CSV:
                consensus.CSVHeight = int{height};
                break;
            }
        }

        for (const auto& [deployment_pos, version_bits_params] : opts.version_bits_parameters) {
            consensus.vDeployments[deployment_pos].nStartTime = version_bits_params.start_time;
            consensus.vDeployments[deployment_pos].nTimeout = version_bits_params.timeout;
            consensus.vDeployments[deployment_pos].min_activation_height = version_bits_params.min_activation_height;
        }

        // BlakeBitcoin regtest reuses the BlakeBitcoin mainnet genesis.
        genesis = CreateBlakeBitcoinGenesisBlock(1399109785, 183893667, 503382015, 112, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000dcb4434e2148558a0a5c71e5c06d864accef97d75ac1c031405deb3371"));
        assert(genesis.hashMerkleRoot == uint256S("0x0423141660220f9f155a4129a49dcb6431bbed9cd037bba3da34c2baa53ed0ac"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();
        vSeeds.emplace_back("dummySeed.invalid.");

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256S("0x000000dcb4434e2148558a0a5c71e5c06d864accef97d75ac1c031405deb3371")},
            }
        };

        // BlakeBitcoin regtest assumeutxo snapshot at height 110. Captured by
        // running the regtest 110-block TestChain100Setup sequence and dumping
        // the resulting snapshot's txoutset_hash.
        m_assumeutxo_data = MapAssumeutxo{
            {
                110,
                {
                    AssumeutxoHash{uint256S("0x01a89c199e5800105a7d6ea026c5985eacde9e684f33e32beab24c3039f10547")},
                    111,
                },
            },
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,243);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,7);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "rbbtc";
    }
};

std::unique_ptr<const CChainParams> CChainParams::SigNet(const SigNetOptions& options)
{
    return std::make_unique<const SigNetParams>(options);
}

std::unique_ptr<const CChainParams> CChainParams::RegTest(const RegTestOptions& options)
{
    return std::make_unique<const CRegTestParams>(options);
}

std::unique_ptr<const CChainParams> CChainParams::Main()
{
    return std::make_unique<const CMainParams>();
}

std::unique_ptr<const CChainParams> CChainParams::TestNet()
{
    return std::make_unique<const CTestNetParams>();
}
