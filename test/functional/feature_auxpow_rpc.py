#!/usr/bin/env python3
# Copyright (c) 2026 The BlakeBitcoin Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BlakeBitcoin AuxPoW mining RPCs."""

from test_framework.auxpow import build_auxpow
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class AuxpowRPCTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def assert_auxpow_version_fields(self, work):
        assert_equal(isinstance(work["version"], int), True)
        assert_equal(work["versionHex"], f"{work['version'] & 0xffffffff:08x}")

    def run_test(self):
        node = self.nodes[0]
        address = node.getnewaddress("", "legacy")

        self.log.info("createauxblock returns BlakeBitcoin AuxPoW work")
        start_height = node.getblockcount()
        work = node.createauxblock(address)
        assert_equal(work["chainid"], 5)
        assert_equal(work["height"], start_height + 1)
        assert_equal(work["previousblockhash"], node.getbestblockhash())
        assert_equal(len(work["hash"]), 64)
        assert_equal(len(work["target"]), 64)
        self.assert_auxpow_version_fields(work)

        self.log.info("getauxblock without configured payout address is rejected")
        assert_raises_rpc_error(
            -8,
            "getauxblock without parameters requires -auxpowmineraddress",
            node.getauxblock,
        )

        self.log.info("getauxblock no-arg mode uses -auxpowmineraddress")
        self.restart_node(0, extra_args=[f"-auxpowmineraddress={address}"])
        node = self.nodes[0]
        work = node.getauxblock()
        assert_equal(work["chainid"], 5)
        assert_equal(work["height"], start_height + 1)
        self.assert_auxpow_version_fields(work)

        self.log.info("malformed AuxPoW payload is rejected")
        assert_raises_rpc_error(-22, "AuxPoW decode failed", node.submitauxblock, work["hash"], "00")
        assert_raises_rpc_error(-22, "trailing data", node.submitauxblock, work["hash"], build_auxpow(work["hash"], 0) + "00")
        assert_raises_rpc_error(-8, "block hash unknown", node.submitauxblock, "0" * 64, build_auxpow(work["hash"], 0))

        self.log.info("submitauxblock accepts a valid regtest AuxPoW payload")
        accepted = False
        solved_auxpow = None
        for nonce in range(128):
            auxpow = build_auxpow(work["hash"], nonce)
            if node.submitauxblock(work["hash"], auxpow):
                accepted = True
                solved_auxpow = auxpow
                break
        assert_equal(accepted, True)
        assert_equal(node.getblockcount(), start_height + 1)
        assert_equal(node.getbestblockhash(), work["hash"])

        self.log.info("getauxblock submit mode reports duplicates as false")
        assert_equal(node.getauxblock(work["hash"], solved_auxpow), False)

        self.log.info("restart preserves the AuxPoW block from blk*.dat")
        self.restart_node(0)
        node = self.nodes[0]
        assert_equal(node.getblockcount(), start_height + 1)
        assert_equal(node.getbestblockhash(), work["hash"])
        block = node.getblock(work["hash"])
        assert_equal(block["hash"], work["hash"])
        assert_equal(block["height"], start_height + 1)
        assert_equal(block["confirmations"], 1)

        self.log.info("-reindex reloads the AuxPoW block from blk*.dat")
        self.restart_node(0, extra_args=["-reindex"])
        node = self.nodes[0]
        assert_equal(node.getblockcount(), start_height + 1)
        assert_equal(node.getbestblockhash(), work["hash"])
        block = node.getblock(work["hash"])
        assert_equal(block["hash"], work["hash"])
        assert_equal(block["height"], start_height + 1)


if __name__ == "__main__":
    AuxpowRPCTest().main()
