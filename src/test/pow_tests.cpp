// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

static uint32_t CompactTarget(const arith_uint256& target)
{
    return target.GetCompact();
}

static arith_uint256 TargetFromCompact(uint32_t compact)
{
    arith_uint256 target;
    target.SetCompact(compact);
    return target;
}

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto& consensus = chainParams->GetConsensus();
    int64_t nLastRetargetTime = 1'000'000;
    CBlockIndex pindexLast;
    pindexLast.nHeight = consensus.DifficultyAdjustmentInterval() - 1;
    pindexLast.nTime = nLastRetargetTime + consensus.nPowTargetTimespan * 3 / 2;
    const arith_uint256 old_target = UintToArith256(consensus.powLimit) / 4;
    pindexLast.nBits = CompactTarget(old_target);

    arith_uint256 expected_target = TargetFromCompact(pindexLast.nBits);
    expected_target *= 3;
    expected_target /= 2;
    const uint32_t expected_nbits = CompactTarget(expected_target);
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, consensus), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(consensus, pindexLast.nHeight + 1, pindexLast.nBits, expected_nbits));
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto& consensus = chainParams->GetConsensus();
    int64_t nLastRetargetTime = 2'000'000;
    CBlockIndex pindexLast;
    pindexLast.nHeight = consensus.DifficultyAdjustmentInterval() - 1;
    pindexLast.nTime = nLastRetargetTime + consensus.nPowTargetTimespan * 8;
    pindexLast.nBits = CompactTarget(UintToArith256(consensus.powLimit));
    const uint32_t expected_nbits = CompactTarget(UintToArith256(consensus.powLimit));
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, consensus), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(consensus, pindexLast.nHeight + 1, pindexLast.nBits, expected_nbits));
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto& consensus = chainParams->GetConsensus();
    int64_t nLastRetargetTime = 3'000'000;
    CBlockIndex pindexLast;
    pindexLast.nHeight = consensus.DifficultyAdjustmentInterval() - 1;
    pindexLast.nTime = nLastRetargetTime + consensus.nPowTargetTimespan / 10;
    const arith_uint256 old_target = UintToArith256(consensus.powLimit) / 8;
    pindexLast.nBits = CompactTarget(old_target);

    arith_uint256 expected_target = TargetFromCompact(pindexLast.nBits);
    expected_target /= 4;
    const uint32_t expected_nbits = CompactTarget(expected_target);
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, consensus), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(consensus, pindexLast.nHeight + 1, pindexLast.nBits, expected_nbits));

    const uint32_t invalid_nbits = CompactTarget(TargetFromCompact(pindexLast.nBits) / 5);
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, pindexLast.nHeight + 1, pindexLast.nBits, invalid_nbits));
}

BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual_post_3500)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto& consensus = chainParams->GetConsensus();
    int64_t nLastRetargetTime = 3'500'000;
    CBlockIndex pindexLast;
    pindexLast.nHeight = consensus.DifficultyAdjustmentInterval() * 19 - 1;
    pindexLast.nTime = nLastRetargetTime + consensus.nPowTargetTimespan / 10;
    const arith_uint256 old_target = UintToArith256(consensus.powLimit) / 8;
    pindexLast.nBits = CompactTarget(old_target);

    arith_uint256 expected_target = TargetFromCompact(pindexLast.nBits);
    expected_target /= 4;
    const uint32_t expected_nbits = CompactTarget(expected_target);
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, consensus), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(consensus, pindexLast.nHeight + 1, pindexLast.nBits, expected_nbits));

    const uint32_t quarter_timespan_nbits = CompactTarget(TargetFromCompact(pindexLast.nBits) / 4);
    BOOST_CHECK(PermittedDifficultyTransition(consensus, pindexLast.nHeight + 1, pindexLast.nBits, quarter_timespan_nbits));

    const uint32_t invalid_nbits = CompactTarget(TargetFromCompact(pindexLast.nBits) / 5);
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, pindexLast.nHeight + 1, pindexLast.nBits, invalid_nbits));
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    const auto& consensus = chainParams->GetConsensus();
    int64_t nLastRetargetTime = 4'000'000;
    CBlockIndex pindexLast;
    pindexLast.nHeight = consensus.DifficultyAdjustmentInterval() - 1;
    pindexLast.nTime = nLastRetargetTime + consensus.nPowTargetTimespan * 10;
    const arith_uint256 old_target = UintToArith256(consensus.powLimit) / 64;
    pindexLast.nBits = CompactTarget(old_target);

    const arith_uint256 expected_target = TargetFromCompact(pindexLast.nBits) * 4;
    const uint32_t expected_nbits = CompactTarget(expected_target);
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, consensus), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(consensus, pindexLast.nHeight + 1, pindexLast.nBits, expected_nbits));

    const uint32_t invalid_nbits = CompactTarget(TargetFromCompact(pindexLast.nBits) * 5);
    BOOST_CHECK(!PermittedDifficultyTransition(consensus, pindexLast.nHeight + 1, pindexLast.nBits, invalid_nbits));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits{~0x00800000U};
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith = UintToArith256(consensus.powLimit);
    nBits = hash_arith.GetCompact();
    hash_arith *= 2; // hash > nBits
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_zero_target)
{
    const auto consensus = CreateChainParams(*m_node.args, CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

void sanity_check_chainparams(const ArgsManager& args, std::string chainName)
{
    const auto chainParams = CreateChainParams(args, chainName);
    const auto consensus = chainParams->GetConsensus();

    // hash genesis is correct
    BOOST_CHECK_EQUAL(consensus.hashGenesisBlock, chainParams->GenesisBlock().GetHash());

    // target timespan is an even multiple of spacing
    BOOST_CHECK_EQUAL(consensus.nPowTargetTimespan % consensus.nPowTargetSpacing, 0);

    // genesis nBits is positive, doesn't overflow and is lower than powLimit
    arith_uint256 pow_compact;
    bool neg, over;
    pow_compact.SetCompact(chainParams->GenesisBlock().nBits, &neg, &over);
    BOOST_CHECK(!neg && pow_compact != 0);
    BOOST_CHECK(!over);
    BOOST_CHECK(UintToArith256(consensus.powLimit) >= pow_compact);

    // check max target * 2*nPowTargetTimespan doesn't overflow -- see pow.cpp:CalculateNextWorkRequired()
    if (!consensus.fPowNoRetargeting) {
        arith_uint256 targ_max("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
        targ_max /= consensus.nPowTargetTimespan*2;
        BOOST_CHECK(UintToArith256(consensus.powLimit) < targ_max);
    }
}

BOOST_AUTO_TEST_CASE(ChainParams_MAIN_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::MAIN);
}

BOOST_AUTO_TEST_CASE(ChainParams_REGTEST_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::REGTEST);
}

BOOST_AUTO_TEST_CASE(ChainParams_TESTNET_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::TESTNET);
}

BOOST_AUTO_TEST_CASE(ChainParams_SIGNET_sanity)
{
    sanity_check_chainparams(*m_node.args, CBaseChainParams::SIGNET);
}

BOOST_AUTO_TEST_SUITE_END()
