// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double fSigcheckVerificationFactor = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64 nTimeLastCheckpoint;
        int64 nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        ( 0, uint256("0x000000dcb4434e2148558a0a5c71e5c06d864accef97d75ac1c031405deb3371"))
        ( 145025, uint256("0x0316c10a202c2bde44628c8cac2d75d61f078a1d961ae1499eaa98eb643b5068"))
        ( 179266, uint256("0x7b102e1f37971dcd4311cc64f83fc62da0f75c22270e831be0a6c8cc38ddd5c8"))
        ( 338643, uint256("0xdd79a4b1ac2a91d9666d97a2654ee826c84e55495665bacaac2b9a953616f8d6"))
        ( 406644, uint256("0xc0f29fe22936216e6a90a4178967ba8ffa9ad78930aa1a369a6fc727a3d2f8e5"))
        ( 845000, uint256("0xa32d61133e22687a63c0c2769552a851a484b030cda02f8a1def5a506d368e33"))
        ( 900000, uint256("0x1054253e8fd7b596cdddd562619da01022024623ee72ed3b37ea909c3caa5cc7"))
        ( 950000, uint256("0xdd4b0d5c8ae8dca9b0fd8ebee3fdf1312c47cdf73f18bff2379d5e4e2d1e59c8"))
        ( 966300, uint256("0xb4d0943e70a43256e5a329e7f7450abc42cd1aa9a8f277b0ef7a990dccbbe800"))
		( 1004800, uint256("0xea2e486ef9e96a02e8bfa6782cb2aa36b393783af0b4793dcaa58747f70cd71d"))
		( 1352360, uint256("0xadf9abb289d6e69ca373b4c6dc5853a7c444ea6db5bc1e33b6df2c061eb9444a"))
		( 1719100, uint256("0xe1572e3497370fc796c9722f28c570db392d284f574eb652cb953e39b14a0127"))
	;
    static const CCheckpointData data = {
        &mapCheckpoints,
        1646509845, // * UNIX timestamp of last checkpoint block
        2132261,     // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        60000.0     // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet = 
        boost::assign::map_list_of
        ( 0, uint256("0x00000052d978f26d698e0c4dbce9f8139a69f2fbda37715149146776aeb70d5b"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1392351202,
        0,
        60000.0
    };

    const CCheckpointData &Checkpoints() {
        if (fTestNet)
            return dataTestnet;
        else
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!GetBoolArg("-checkpoints", true))
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex) {
        if (pindex==NULL)
            return 0.0;

        int64 nNow = time(NULL);

        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (!GetBoolArg("-checkpoints", true))
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!GetBoolArg("-checkpoints", true))
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
