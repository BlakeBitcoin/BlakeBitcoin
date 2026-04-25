// Copyright (c) 2015 The Bitcoin Core developers
// Copyright (c) 2013-2026 The BlakeBitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "chainparams.h"
#include "pow.h"
#include "random.h"
#include "util.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

// The original Bitcoin Core get_next_work* tests used 2-week retarget timespan
// vectors with powLimit compact 0x1d00ffff. The Blakecoin-family forks here use
// short retarget timespans (1 hour for Blakecoin/Photon/lithium/universalmol,
// 30 minutes for Electron-ELT, 2 weeks for BlakeBitcoin) and powLimit compact
// 0x1e00ffff, so those Bitcoin-specific vectors do not apply. They are removed
// here; the pow_limit_clamp test below validates the clamp behaviour against
// the chain's actual powLimit.

BOOST_AUTO_TEST_CASE(pow_limit_clamp)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& params = chainParams->GetConsensus();

    CBlockIndex pindexLast;
    pindexLast.nHeight = params.nMinerConfirmationWindow - 1;
    pindexLast.nTime = 1500000000;
    pindexLast.nBits = 0x1e00ffff;

    // Far longer than nPowTargetTimespan: retarget should clamp at powLimit.
    const int64_t nLastRetargetTime = pindexLast.nTime - (params.nPowTargetTimespan * 100);
    const unsigned int nextBits = CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, params);

    const unsigned int powLimitCompact = UintToArith256(params.powLimit).GetCompact();
    BOOST_CHECK_EQUAL(nextBits, powLimitCompact);
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
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

BOOST_AUTO_TEST_SUITE_END()
