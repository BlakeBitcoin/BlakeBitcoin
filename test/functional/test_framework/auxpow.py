#!/usr/bin/env python3
# Copyright (c) 2026 The BlakeBitcoin Developers
# Copyright (c) 2026 The BlakeBitcoin Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpers for BlakeBitcoin-family AuxPoW functional tests."""

import hashlib
import struct


def ser_compact_size(size):
    if size < 253:
        return struct.pack("<B", size)
    if size <= 0xffff:
        return b"\xfd" + struct.pack("<H", size)
    if size <= 0xffffffff:
        return b"\xfe" + struct.pack("<I", size)
    return b"\xff" + struct.pack("<Q", size)


def ser_string(data):
    return ser_compact_size(len(data)) + data


def build_auxpow(child_hash, parent_nonce):
    # Mirrors CAuxPow::SERIALIZE_METHODS in src/auxpow.h:
    # coinbase tx, legacy hash_block/tx_index fields, chain merkle branch,
    # chain index, then the pure parent block header.
    root_bytes = bytes.fromhex(child_hash)
    script_sig = ser_string(root_bytes + struct.pack("<I", 1) + struct.pack("<I", 0))

    coinbase = (
        struct.pack("<i", 2)
        + ser_compact_size(1)
        + (b"\x00" * 32)
        + struct.pack("<I", 0xffffffff)
        + script_sig
        + struct.pack("<I", 0xffffffff)
        + ser_compact_size(0)
        + struct.pack("<I", 0)
    )
    coinbase_hash = hashlib.sha256(coinbase).digest()

    parent_header = (
        struct.pack("<i", 1)
        + (b"\x00" * 32)
        + coinbase_hash
        + struct.pack("<I", 0)
        + struct.pack("<I", 0)
        + struct.pack("<I", parent_nonce)
    )

    return (
        coinbase
        + (b"\x00" * 32)
        + ser_compact_size(0)
        + struct.pack("<i", 0)
        + ser_compact_size(0)
        + struct.pack("<I", 0)
        + parent_header
    ).hex()


def solve_auxpow_submit(node, child_hash, submit):
    for nonce in range(128):
        auxpow = build_auxpow(child_hash, nonce)
        if submit(child_hash, auxpow):
            return auxpow
    raise AssertionError(f"unable to solve AuxPoW payload for {child_hash}")
