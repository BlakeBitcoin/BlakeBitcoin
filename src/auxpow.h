// Copyright (c) 2011 Vince Durham
// Copyright (c) 2009-present The Bitcoin Core developers
// Copyright (c) 2013-2026 The BlakeBitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_AUXPOW_H
#define BITCOIN_AUXPOW_H

#include <consensus/params.h>
#include <primitives/pureheader.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <version.h>

#include <cstdint>
#include <memory>
#include <vector>

class CBlockHeader;

static constexpr unsigned char PCH_MERGED_MINING_HEADER[] = {0xfa, 0xbe, 'm', 'm'};

class CAuxPow
{
private:
    CTransactionRef m_coinbase_tx;
    std::vector<uint256> m_merkle_branch;

    static uint256 CheckMerkleBranch(uint256 hash, const std::vector<uint256>& merkle_branch, int index);

public:
    std::vector<uint256> vChainMerkleBranch;
    unsigned int nChainIndex{0};
    CPureBlockHeader parentBlock;

    CAuxPow() = default;
    explicit CAuxPow(CTransactionRef tx) : m_coinbase_tx{std::move(tx)} {}

    SERIALIZE_METHODS(CAuxPow, obj)
    {
        uint256 hash_block;
        int tx_index{0};

        if constexpr (ser_action.ForRead()) {
            OverrideStream<Stream> tx_stream(&s, SER_NETWORK, PROTOCOL_VERSION);
            tx_stream >> obj.m_coinbase_tx;
        } else {
            OverrideStream<Stream> tx_stream(&s, SER_NETWORK, PROTOCOL_VERSION);
            tx_stream << obj.m_coinbase_tx;
        }
        READWRITE(hash_block, obj.m_merkle_branch, tx_index);
        READWRITE(obj.vChainMerkleBranch, obj.nChainIndex, obj.parentBlock);
    }

    bool Check(const uint256& hash_aux_block, int chain_id, const Consensus::Params& params) const;
    uint256 GetParentBlockPoWHash() const { return parentBlock.GetPoWHash(); }

    const CTransactionRef& GetCoinbaseTx() const { return m_coinbase_tx; }
    const std::vector<uint256>& GetCoinbaseMerkleBranch() const { return m_merkle_branch; }

    static int GetExpectedIndex(uint32_t nonce, int chain_id, unsigned height);
    static void InitAuxPow(CBlockHeader& header);
};

#endif // BITCOIN_AUXPOW_H
