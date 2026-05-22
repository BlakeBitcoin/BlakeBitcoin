// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Copyright (c) 2013-2026 The BlakeBitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_PUREHEADER_H
#define BITCOIN_PRIMITIVES_PUREHEADER_H

#include <serialize.h>
#include <uint256.h>
#include <util/time.h>

#include <cstdint>

/**
 * A block header without AuxPoW payload.
 *
 * AuxPoW stores a parent block header inside the AuxPoW payload. Splitting the
 * six-field mined header avoids a type cycle and keeps the block hash anchored
 * to the pure 80-byte header.
 */
class CPureBlockHeader
{
public:
    static constexpr int32_t BLOCK_VERSION_DEFAULT = (1 << 4);
    static constexpr int32_t VERSION_AUXPOW = (1 << 8);
    static constexpr int32_t VERSION_CHAIN_START = (1 << 16);
    static constexpr int32_t VERSION_CHAIN_MASK = (0xFF << 16);

    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CPureBlockHeader() { SetNull(); }

    SERIALIZE_METHODS(CPureBlockHeader, obj)
    {
        READWRITE(obj.nVersion, obj.hashPrevBlock, obj.hashMerkleRoot, obj.nTime, obj.nBits, obj.nNonce);
    }

    void SetNull()
    {
        nVersion = BLOCK_VERSION_DEFAULT;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }

    bool IsNull() const { return nBits == 0; }

    uint256 GetHash() const;
    uint256 GetPoWHash() const;

    NodeSeconds Time() const { return NodeSeconds{std::chrono::seconds{nTime}}; }
    int64_t GetBlockTime() const { return static_cast<int64_t>(nTime); }

    int32_t GetBaseVersion() const { return GetBaseVersion(nVersion); }

    static int32_t GetBaseVersion(int32_t version)
    {
        return version & ~VERSION_AUXPOW & ~VERSION_CHAIN_MASK;
    }

    void SetBaseVersion(int32_t base_version, int32_t chain_id)
    {
        const int32_t modifiers = nVersion & VERSION_AUXPOW;
        nVersion = (base_version & ~VERSION_AUXPOW & ~VERSION_CHAIN_MASK) |
                   modifiers |
                   ((chain_id & 0xFF) * VERSION_CHAIN_START);
    }

    int32_t GetChainId() const { return (nVersion & VERSION_CHAIN_MASK) / VERSION_CHAIN_START; }

    void SetChainId(int32_t chain_id)
    {
        nVersion &= ~VERSION_CHAIN_MASK;
        nVersion |= (chain_id & 0xFF) * VERSION_CHAIN_START;
    }

    bool IsAuxpow() const { return nVersion & VERSION_AUXPOW; }

    void SetAuxpowFlag(bool auxpow)
    {
        if (auxpow) {
            nVersion |= VERSION_AUXPOW;
        } else {
            nVersion &= ~VERSION_AUXPOW;
        }
    }
};

#endif // BITCOIN_PRIMITIVES_PUREHEADER_H
