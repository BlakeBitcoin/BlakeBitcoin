// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Copyright (c) 2013-2026 The BlakeBitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/pureheader.h>

#include <hash.h>

uint256 CPureBlockHeader::GetHash() const
{
    return Hashblake(BEGIN(nVersion), END(nNonce));
}

uint256 CPureBlockHeader::GetPoWHash() const
{
    return GetHash();
}
