#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test blockmaxconflict - block wont automatic merge if there are too many blocks conflicts.

Tests correspond to code in validation.cpp 
"""

from decimal import Decimal
import http.client
import subprocess
import time

from test_framework.test_framework import FabcoinTestFramework
from test_framework.util import *
from test_framework.fabcoinconfig import *

from test_framework.util import (
    assert_equal,
    assert_raises,
    assert_raises_rpc_error,
    assert_is_hex_string,
    assert_is_hash_string,
)

class BlockConflictTest(FabcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True
        str_args = '-blockmaxconflict=%d'%(30)
        #node0 and node1 apply block frozen logic, node2 wont, will alway follow longest chain.
        self.extra_args = [[str_args],[str_args],[]]

    def setup_network(self, split=False):
        self.setup_nodes()
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[2], 0)
        self.sync_all()

    def run_test(self):
        #initial generate 900
        self.nodes[0].generate(450)
        self.sync_all()
        self.nodes[1].generate(450)
        self.sync_all()
        assert_equal(self.nodes[0].getinfo()['blocks'], COINBASE_MATURITY+100)
        assert_equal(self.nodes[1].getinfo()['blocks'], COINBASE_MATURITY+100)
        assert_equal(self.nodes[2].getinfo()['blocks'], COINBASE_MATURITY+100)

        # two node connected, generate total 100
        self.nodes[0].generate(25)
        self.sync_all()
        self.nodes[1].generate(25)
        self.sync_all()
        self.nodes[0].generate(25)
        self.sync_all()
        self.nodes[1].generate(25)
        self.sync_all()

        assert_equal(self.nodes[0].getinfo()['blocks'], COINBASE_MATURITY+100+100)
        assert_equal(self.nodes[1].getinfo()['blocks'], COINBASE_MATURITY+100+100)
        assert_equal(self.nodes[2].getinfo()['blocks'], COINBASE_MATURITY+100+100)

        # two node disconnect, one generate 100, another generate 29 , lower than blockmaxconflict, short chain be discharged. 
        disconnect_nodes(self.nodes[0],1)
        disconnect_nodes(self.nodes[1],2)
        disconnect_nodes(self.nodes[2],0)

        self.nodes[0].generate(100)
        self.nodes[1].generate(29)

        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[2], 0)
        sync_blocks(self.nodes)
 
        assert_equal(self.nodes[0].getinfo()['blocks'], COINBASE_MATURITY+100+100+100)
        assert_equal(self.nodes[1].getinfo()['blocks'], COINBASE_MATURITY+100+100+100)   # short chain accept longer chain
        assert_equal(self.nodes[2].getinfo()['blocks'], COINBASE_MATURITY+100+100+100)   # short chain accept longest chain -without blocmaxconflict

        # two node disconnect, one generate 100, another generate 30 , larger or equal to  blockmaxconflict, short chain wont accept longer chain. 
        disconnect_nodes(self.nodes[0],1)
        disconnect_nodes(self.nodes[1],2)
        disconnect_nodes(self.nodes[0],2)

        self.nodes[0].generate(100)
        self.nodes[1].generate(30)

        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[0], 2)
        time.sleep(10)
 
        assert_equal(self.nodes[0].getinfo()['blocks'], COINBASE_MATURITY+100+100+100+100)
        assert_equal(self.nodes[1].getinfo()['blocks'], COINBASE_MATURITY+100+100+100+ 30)    # short chain reject longer chain - which beyond blockmaxconflict limit
        assert_equal(self.nodes[2].getinfo()['blocks'], COINBASE_MATURITY+100+100+100+100)   # short chain accept longest chain - without blockconflict

        # hard fork happen, two chain could mining seperate
        self.nodes[0].generate(100)
        self.nodes[1].generate(50)

        time.sleep(10)
 
        assert_equal(self.nodes[0].getinfo()['blocks'], COINBASE_MATURITY+100+100+100+100+100)
        assert_equal(self.nodes[1].getinfo()['blocks'], COINBASE_MATURITY+100+100+100+ 30+ 50)    # short chain reject longer chain - which beyond blockmaxconflict limit
        assert_equal(self.nodes[2].getinfo()['blocks'], COINBASE_MATURITY+100+100+100+100+100)    # short chain accept longest chain - without blockconflict


        # hard fork happen, two chain could mining independ 
        self.nodes[0].generate(100)
        self.nodes[1].generate(300)
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[2], 1)
        connect_nodes(self.nodes[2], 0)

        time.sleep(10)
 
        assert_equal(self.nodes[0].getinfo()['blocks'], COINBASE_MATURITY+100+100+100+100+100+100)
        assert_equal(self.nodes[1].getinfo()['blocks'], COINBASE_MATURITY+100+100+100+ 30+ 50+300)    # short chain reject longer chain - which beyond blockmaxconflict limit
        assert_equal(self.nodes[2].getinfo()['blocks'], COINBASE_MATURITY+100+100+100+ 30+ 50+300)    # short chain accept longest chain - without blockconflict


if __name__ == '__main__':
    BlockConflictTest().main()




