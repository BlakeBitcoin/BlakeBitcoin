#!/usr/bin/env python3
# Copyright (c) 2026 The BlakeBitcoin Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test AuxPoW RPC mining of SegWit spends at activation."""

from decimal import Decimal

from test_framework.auxpow import solve_auxpow_submit
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


AUXPOW_VERSION_BIT = 1 << 8
VERSIONBITS_TOP_BITS = 0x20000000
SEGWIT_ACTIVATION_HEIGHT = 600
WITNESS_COMMITMENT_HEADER = "6a24aa21a9ed"


class AuxpowSegwitMiningTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[
            "-acceptnonstdtxn=1",
            f"-testactivationheight=segwit@{SEGWIT_ACTIVATION_HEIGHT}",
            "-addresstype=legacy",
        ]]
        self.rpc_timeout = 120

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def mine_auxpow_block(self, node):
        payout_address = node.getnewaddress("", "legacy")
        work = node.createauxblock(payout_address)
        solve_auxpow_submit(node, work["hash"], node.submitauxblock)
        assert_equal(node.getbestblockhash(), work["hash"])
        return work["hash"]

    def mine_blocks(self, node, count):
        return node.generatetoaddress(count, node.getnewaddress("", "legacy"), invalid_call=False)

    def get_coinbase_tx(self, node, block_hash):
        block = node.getblock(block_hash)
        return node.getrawtransaction(block["tx"][0], True, block_hash)

    def has_witness_commitment(self, tx):
        return any(
            vout["scriptPubKey"]["hex"].startswith(WITNESS_COMMITMENT_HEADER)
            for vout in tx["vout"]
        )

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Mine spendable pre-activation balance")
        self.mine_blocks(node, 500)
        assert node.getblockcount() < SEGWIT_ACTIVATION_HEIGHT - 1

        self.log.info("Mine a pre-activation legacy funding tx through AuxPoW")
        segwit_address = node.getnewaddress("", "bech32")
        funding_amount = min(Decimal("1.0"), node.getbalance() / 2).quantize(Decimal("0.00000001"))
        assert funding_amount > Decimal("0.001")
        funding_txid = node.sendtoaddress(segwit_address, funding_amount)
        pre_activation_hash = self.mine_auxpow_block(node)
        assert_equal(node.getblockcount(), 501)
        assert funding_txid in node.getblock(pre_activation_hash)["tx"]

        pre_coinbase = self.get_coinbase_tx(node, pre_activation_hash)
        assert "txinwitness" not in pre_coinbase["vin"][0]

        self.log.info("Create a signed witness spend before activation")
        funding_utxo = next(
            utxo for utxo in node.listunspent(1, 9999999, [segwit_address])
            if utxo["txid"] == funding_txid
        )
        destination = node.getnewaddress("", "legacy")
        raw_tx = node.createrawtransaction(
            [{"txid": funding_txid, "vout": funding_utxo["vout"]}],
            {destination: funding_amount - Decimal("0.00010000")},
        )
        signed_tx = node.signrawtransactionwithwallet(raw_tx)
        assert_equal(signed_tx["complete"], True)
        decoded_tx = node.decoderawtransaction(signed_tx["hex"])
        assert "txinwitness" in decoded_tx["vin"][0]

        self.log.info("Submit the witness spend for the first SegWit-active block")
        self.mine_blocks(node, SEGWIT_ACTIVATION_HEIGHT - 1 - node.getblockcount())
        assert_equal(node.getblockcount(), SEGWIT_ACTIVATION_HEIGHT - 1)
        witness_txid = node.sendrawtransaction(signed_tx["hex"])
        assert_equal(node.getrawmempool(), [witness_txid])

        self.log.info("Mine the witness spend through AuxPoW")
        segwit_auxpow_hash = self.mine_auxpow_block(node)
        assert_equal(node.getblockcount(), SEGWIT_ACTIVATION_HEIGHT)

        block = node.getblock(segwit_auxpow_hash)
        assert witness_txid in block["tx"]

        mined_tx = node.getrawtransaction(witness_txid, True, segwit_auxpow_hash)
        assert "txinwitness" in mined_tx["vin"][0]
        assert len(mined_tx["vin"][0]["txinwitness"]) > 0

        coinbase_tx = self.get_coinbase_tx(node, segwit_auxpow_hash)
        assert_equal(coinbase_tx["vin"][0]["txinwitness"], ["00" * 32])
        assert_equal(self.has_witness_commitment(coinbase_tx), True)

        header = node.getblockheader(segwit_auxpow_hash)
        assert header["version"] & AUXPOW_VERSION_BIT
        assert_equal(header["version"] & VERSIONBITS_TOP_BITS, VERSIONBITS_TOP_BITS)


if __name__ == "__main__":
    AuxpowSegwitMiningTest().main()
