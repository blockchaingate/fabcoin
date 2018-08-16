#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import FabcoinTestFramework
from test_framework.fabcoinconfig import INITIAL_BLOCK_REWARD, COINBASE_MATURITY
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
import sys

class FabcoinCallContractStateNotRevertedTest(FabcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def run_test(self):
        connect_nodes_bi(self.nodes, 0, 1)
        self.nodes[0].generate(COINBASE_MATURITY+100)
        self.sync_all()
        self.nodes[1].generate(COINBASE_MATURITY+100)
        self.sync_all()
        contract_address = self.nodes[0].createcontract("00")['address']
        self.nodes[0].generate(1)
        self.sync_all()
        self.nodes[0].callcontract(contract_address, "00")
        self.nodes[1].createcontract("00")
        self.nodes[1].generate(1)
        time.sleep(1)
        assert_equal(self.nodes[0].getblockcount(), self.nodes[1].getblockcount())
        assert_equal(self.nodes[0].listcontracts(), self.nodes[1].listcontracts())


if __name__ == '__main__':
    FabcoinCallContractStateNotRevertedTest().main()
