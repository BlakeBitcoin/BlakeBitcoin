#!/usr/bin/env python3
# Copyright (c) 2026 The BlakeBitcoin Developers
# Copyright (c) 2026 The BlakeBitcoin Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BlakeBitcoin AuxPoW header service after block data is pruned."""

from test_framework.auxpow import solve_auxpow_submit
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class AuxpowPrunedHeaderTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-fastprune", "-prune=1"]]
        self.rpc_timeout = 120

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        address = node.getnewaddress("", "legacy")

        self.log.info("Accept an AuxPoW block and record its full serialized header")
        work = node.createauxblock(address)
        solve_auxpow_submit(node, work["hash"], node.submitauxblock)
        auxpow_hash = work["hash"]
        raw_header = node.getblockheader(auxpow_hash, False)
        assert len(raw_header) > 160
        assert_equal(node.getblockheader(auxpow_hash)["height"], 1)

        self.log.info("Mine enough native blocks for fast manual pruning")
        node.generatetoaddress(1000, node.getnewaddress("", "legacy"), invalid_call=False)
        assert_equal(node.getblockcount(), 1001)

        self.log.info("Prune the block file that contained the AuxPoW block body")
        node.pruneblockchain(500)
        assert_equal(node.getblockchaininfo()["pruned"], True)
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, auxpow_hash)

        self.log.info("Pruned node still serves the AuxPoW header from CDiskBlockIndex")
        assert_equal(node.getblockheader(auxpow_hash, False), raw_header)
        assert_equal(node.getblockheader(auxpow_hash)["height"], 1)

        self.log.info("Restart preserves the pruned AuxPoW header service")
        self.restart_node(0, extra_args=["-fastprune", "-prune=1"])
        node = self.nodes[0]
        assert_equal(node.getblockheader(auxpow_hash, False), raw_header)
        assert_equal(node.getblockheader(auxpow_hash)["height"], 1)
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, auxpow_hash)


if __name__ == "__main__":
    AuxpowPrunedHeaderTest().main()
