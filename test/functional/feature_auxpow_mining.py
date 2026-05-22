#!/usr/bin/env python3
# Copyright (c) 2026 The BlakeBitcoin Developers
# Copyright (c) 2026 The BlakeBitcoin Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BlakeBitcoin AuxPoW mining workflow and work-cache behaviour."""

from test_framework.auxpow import build_auxpow, solve_auxpow_submit
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class AuxpowMiningWorkflowTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node0, node1 = self.nodes
        address0 = node0.getnewaddress("", "legacy")
        address1 = node1.getnewaddress("", "legacy")

        self.log.info("createauxblock validates payout addresses")
        assert_raises_rpc_error(-5, "Invalid BlakeBitcoin address", node0.createauxblock, "not-a-blakebitcoin-address")

        self.log.info("createauxblock returns BlakeBitcoin-shaped work")
        start_height = node0.getblockcount()
        work0 = node0.createauxblock(address0)
        assert_equal(work0["chainid"], 5)
        assert_equal(work0["height"], start_height + 1)
        assert_equal(work0["previousblockhash"], node0.getbestblockhash())
        assert_equal(work0["previousblockhash"], node1.getbestblockhash())

        self.log.info("repeated requests return distinct cached work for the same parent")
        work1 = node0.createauxblock(address0)
        assert_equal(work1["height"], work0["height"])
        assert_equal(work1["previousblockhash"], work0["previousblockhash"])
        assert work1["hash"] != work0["hash"]

        self.log.info("different payout scripts produce different work")
        work2 = node0.createauxblock(address1)
        assert_equal(work2["height"], work0["height"])
        assert_equal(work2["previousblockhash"], work0["previousblockhash"])
        assert work2["hash"] not in {work0["hash"], work1["hash"]}

        self.log.info("submitauxblock accepts solved work and propagates it to the peer")
        solved_auxpow = solve_auxpow_submit(node0, work2["hash"], node0.submitauxblock)
        self.sync_blocks()
        assert_equal(node0.getblockcount(), start_height + 1)
        assert_equal(node1.getblockcount(), start_height + 1)
        assert_equal(node0.getbestblockhash(), work2["hash"])
        assert_equal(node1.getbestblockhash(), work2["hash"])
        raw_header = node0.getblockheader(work2["hash"], False)
        assert len(raw_header) > 160
        assert_equal(node1.getblockheader(work2["hash"], False), raw_header)

        self.log.info("duplicate submit through getauxblock compatibility mode returns false")
        assert_equal(node0.getauxblock(work2["hash"], solved_auxpow), False)

        self.log.info("new-tip work clears cached work from the old parent")
        work3 = node0.createauxblock(address0)
        assert_equal(work3["height"], start_height + 2)
        assert_equal(work3["previousblockhash"], work2["hash"])
        assert_raises_rpc_error(-8, "block hash unknown", node0.submitauxblock, work0["hash"], build_auxpow(work0["hash"], 0))

        self.log.info("new-tip work remains mineable")
        solve_auxpow_submit(node0, work3["hash"], node0.submitauxblock)
        self.sync_blocks()
        assert_equal(node0.getblockcount(), start_height + 2)
        assert_equal(node1.getblockcount(), start_height + 2)
        assert_equal(node0.getbestblockhash(), work3["hash"])
        assert_equal(node1.getbestblockhash(), work3["hash"])


if __name__ == "__main__":
    AuxpowMiningWorkflowTest().main()
