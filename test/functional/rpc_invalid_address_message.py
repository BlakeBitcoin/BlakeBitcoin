#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test error messages for 'getaddressinfo' and 'validateaddress' RPC commands."""

from test_framework.segwit_addr import encode_segwit_address
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


def mutate_last_char(addr):
    replacement = 'q' if addr[-1] != 'q' else 'p'
    return addr[:-1] + replacement


class InvalidAddressErrorMessageTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def check_valid(self, addr):
        info = self.nodes[0].validateaddress(addr)
        assert info['isvalid']
        assert 'error' not in info
        assert 'error_locations' not in info

    def check_invalid(self, addr, error_str, error_locations=None):
        res = self.nodes[0].validateaddress(addr)
        assert not res['isvalid']
        assert_equal(res['error'], error_str)
        if error_locations is not None:
            assert_equal(res['error_locations'], error_locations)

    def run_test(self):
        node = self.nodes[0]

        bech32_valid = encode_segwit_address("rbbtc", 0, bytes.fromhex("751e76e8199196d454941c45d1b3a323f1433bd6"))
        bech32_valid_caps = bech32_valid.upper()
        bech32_valid_v1 = encode_segwit_address("rbbtc", 1, bytes.fromhex("79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"))
        base58_valid = node.get_deterministic_priv_key().address

        bech32_invalid_prefix = bech32_valid.replace("rbbtc1", "bc1", 1)
        bech32_invalid_checksum = mutate_last_char(bech32_valid)
        bech32_invalid_checksum_v1 = mutate_last_char(bech32_valid_v1)
        base58_invalid_prefix = '17VZNX1SN5NtKa8UQFxwQbFeFc3iqRYhem'
        base58_invalid_checksum = mutate_last_char(base58_valid)
        invalid_address = 'asfah14i8fajz0123f'

        self.check_invalid(bech32_invalid_prefix, 'Invalid or unsupported Segwit (Bech32) or Base58 encoding.')
        self.check_invalid(bech32_invalid_checksum, 'Invalid Bech32 checksum')
        self.check_invalid(bech32_invalid_checksum_v1, 'Invalid Bech32m checksum')
        self.check_valid(bech32_valid)
        self.check_valid(bech32_valid_caps)
        self.check_valid(bech32_valid_v1)

        self.check_invalid(base58_invalid_prefix, 'Invalid checksum or length of Base58 address (P2PKH or P2SH)')
        self.check_invalid(base58_invalid_checksum, 'Invalid checksum or length of Base58 address (P2PKH or P2SH)')
        self.check_valid(base58_valid)

        self.check_invalid(invalid_address, 'Invalid or unsupported Segwit (Bech32) or Base58 encoding.')

        assert_raises_rpc_error(-1, "Return information about the given bitcoin address.", node.validateaddress)
        assert_raises_rpc_error(-3, "JSON value of type null is not of expected type string", node.validateaddress, None)

        if self.is_wallet_compiled():
            self.init_wallet(node=0)
            assert_raises_rpc_error(-5, 'Invalid or unsupported Segwit (Bech32) or Base58 encoding.', node.getaddressinfo, bech32_invalid_prefix)
            assert_raises_rpc_error(-5, 'Invalid checksum or length of Base58 address (P2PKH or P2SH)', node.getaddressinfo, base58_invalid_prefix)
            assert_raises_rpc_error(-5, 'Invalid or unsupported Segwit (Bech32) or Base58 encoding.', node.getaddressinfo, invalid_address)


if __name__ == '__main__':
    InvalidAddressErrorMessageTest().main()
