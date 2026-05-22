#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test validateaddress for BlakeBitcoin main chain."""

from test_framework.segwit_addr import encode_segwit_address
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


def mutate_last_char(addr):
    replacement = 'q' if addr[-1] != 'q' else 'p'
    return addr[:-1] + replacement


class ValidateAddressMainTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.chain = ""  # main
        self.num_nodes = 1
        self.extra_args = [["-prune=899"]] * self.num_nodes

    def check_valid(self, addr, spk):
        info = self.nodes[0].validateaddress(addr)
        assert_equal(info["isvalid"], True)
        assert_equal(info["scriptPubKey"], spk)
        assert "error" not in info
        assert "error_locations" not in info

    def check_invalid(self, addr, error_str, error_locations=None):
        res = self.nodes[0].validateaddress(addr)
        assert_equal(res["isvalid"], False)
        assert_equal(res["error"], error_str)
        if error_locations is not None:
            assert_equal(res["error_locations"], error_locations)

    def run_test(self):
        valid_v0 = encode_segwit_address("bbtc", 0, bytes.fromhex("751e76e8199196d454941c45d1b3a323f1433bd6"))
        valid_v1 = encode_segwit_address("bbtc", 1, bytes.fromhex("79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"))
        valid_v0_caps = valid_v0.upper()

        self.check_valid(valid_v0, "0014751e76e8199196d454941c45d1b3a323f1433bd6")
        self.check_valid(valid_v0_caps, "0014751e76e8199196d454941c45d1b3a323f1433bd6")
        self.check_valid(valid_v1, "512079be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798")

        self.check_invalid(valid_v0.replace("bbtc1", "bc1", 1), "Invalid or unsupported Segwit (Bech32) or Base58 encoding.")
        self.check_invalid(valid_v1.replace("bbtc1", "tb1", 1), "Invalid or unsupported Segwit (Bech32) or Base58 encoding.")
        self.check_invalid(mutate_last_char(valid_v0), "Invalid Bech32 checksum")
        self.check_invalid(mutate_last_char(valid_v1), "Invalid Bech32m checksum")


if __name__ == "__main__":
    ValidateAddressMainTest().main()
