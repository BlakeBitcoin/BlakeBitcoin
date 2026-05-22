#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the deriveaddresses rpc call."""
from blake256 import blake256_hash

from test_framework.address import (
    bech32_to_bytes,
    byte_to_base58,
    byte_to_base58_with_checksum,
    legacy_base58_to_byte,
    program_to_witness,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create
from test_framework.util import assert_equal, assert_raises_rpc_error


TESTNET_TO_BLAKE_EXTKEY = {
    bytes.fromhex("043587cf"): bytes.fromhex("0488b21e"),
    bytes.fromhex("04358394"): bytes.fromhex("0488ade4"),
}


def translate_testnet_extkey(extkey):
    payload, version = legacy_base58_to_byte(extkey, version_length=4)
    return byte_to_base58_with_checksum(payload, TESTNET_TO_BLAKE_EXTKEY[version], blake256_hash)


def translate_testnet_address(address):
    witver, witprog = bech32_to_bytes(address)
    if witver is not None:
        return program_to_witness(witver, witprog)

    payload, version = legacy_base58_to_byte(address)
    if version == 111:
        return byte_to_base58(payload, 243)
    if version == 196:
        return byte_to_base58(payload, 7)
    raise AssertionError(f"Unexpected legacy fixture version {version} for address {address}")


class DeriveaddressesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        assert_raises_rpc_error(-5, "Missing checksum", self.nodes[0].deriveaddresses, "a")

        xpriv = translate_testnet_extkey("tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK")
        xpub = translate_testnet_extkey("tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B")
        descriptor = descsum_create(f"wpkh({xpriv}/1/1/0)")
        address = translate_testnet_address("bcrt1qjqmxmkpmxt80xz4y3746zgt0q3u3ferr34acd5")
        assert_equal(self.nodes[0].deriveaddresses(descriptor), [address])

        descriptor = descriptor[:-9]
        assert_raises_rpc_error(-5, "Missing checksum", self.nodes[0].deriveaddresses, descriptor)

        descriptor_pubkey = descsum_create(f"wpkh({xpub}/1/1/0)")
        assert_equal(self.nodes[0].deriveaddresses(descriptor_pubkey), [address])

        ranged_descriptor = descsum_create(f"wpkh({xpriv}/1/1/*)")
        ranged_addresses = [
            translate_testnet_address("bcrt1qhku5rq7jz8ulufe2y6fkcpnlvpsta7rq4442dy"),
            translate_testnet_address("bcrt1qpgptk2gvshyl0s9lqshsmx932l9ccsv265tvaq"),
        ]
        assert_equal(self.nodes[0].deriveaddresses(ranged_descriptor, [1, 2]), ranged_addresses)
        assert_equal(self.nodes[0].deriveaddresses(ranged_descriptor, 2), [address] + ranged_addresses)

        assert_raises_rpc_error(-8, "Range should not be specified for an un-ranged descriptor", self.nodes[0].deriveaddresses, descsum_create(f"wpkh({xpriv}/1/1/0)"), [0, 2])

        assert_raises_rpc_error(-8, "Range must be specified for a ranged descriptor", self.nodes[0].deriveaddresses, descsum_create(f"wpkh({xpriv}/1/1/*)"))

        assert_raises_rpc_error(-8, "End of range is too high", self.nodes[0].deriveaddresses, descsum_create(f"wpkh({xpriv}/1/1/*)"), 10000000000)

        assert_raises_rpc_error(-8, "Range is too large", self.nodes[0].deriveaddresses, descsum_create(f"wpkh({xpriv}/1/1/*)"), [1000000000, 2000000000])

        assert_raises_rpc_error(-8, "Range specified as [begin,end] must not have begin after end", self.nodes[0].deriveaddresses, descsum_create(f"wpkh({xpriv}/1/1/*)"), [2, 0])

        assert_raises_rpc_error(-8, "Range should be greater or equal than 0", self.nodes[0].deriveaddresses, descsum_create(f"wpkh({xpriv}/1/1/*)"), [-1, 0])

        combo_descriptor = descsum_create(f"combo({xpriv}/1/1/0)")
        assert_equal(self.nodes[0].deriveaddresses(combo_descriptor), [
            translate_testnet_address("mtfUoUax9L4tzXARpw1oTGxWyoogp52KhJ"),
            translate_testnet_address("mtfUoUax9L4tzXARpw1oTGxWyoogp52KhJ"),
            address,
            translate_testnet_address("2NDvEwGfpEqJWfybzpKPHF2XH3jwoQV3D7x"),
        ])

        # Before #26275, bitcoind would crash when deriveaddresses was
        # called with derivation index 2147483647, which is the maximum
        # positive value of a signed int32, and - currently - the
        # maximum value that the deriveaddresses bitcoin RPC call
        # accepts as derivation index.
        assert_equal(self.nodes[0].deriveaddresses(descsum_create(f"wpkh({xpriv}/1/1/*)"), [2147483647, 2147483647]), [translate_testnet_address("bcrt1qtzs23vgzpreks5gtygwxf8tv5rldxvvsyfpdkg")])

        hardened_without_privkey_descriptor = descsum_create(f"wpkh({xpub}/1'/1/0)")
        assert_raises_rpc_error(-5, "Cannot derive script without private keys", self.nodes[0].deriveaddresses, hardened_without_privkey_descriptor)

        bare_multisig_descriptor = descsum_create(f"multi(1,{xpub}/1/1/0,{xpub}/1/1/1)")
        assert_raises_rpc_error(-5, "Descriptor does not have a corresponding address", self.nodes[0].deriveaddresses, bare_multisig_descriptor)

if __name__ == '__main__':
    DeriveaddressesTest().main()
