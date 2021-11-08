#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test BIP 9 soft forks.

Connect to a single node.
regtest lock-in with 633/844 block signalling
activation after a further 844 blocks
mine 2 block and save coinbases for later use
mine 841 blocks to transition from DEFINED to STARTED
mine 100 blocks signalling readiness and 44 not in order to fail to change state this period
mine 633 blocks signalling readiness and 211 blocks not signalling readiness (STARTED->LOCKED_IN)
mine a further 843 blocks (LOCKED_IN)
test that enforcement has not triggered (which triggers ACTIVE)
test that enforcement has triggered
"""
from io import BytesIO
import shutil
import time
import itertools

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.mininode import CTransaction, NetworkThread
from test_framework.blocktools import create_coinbase, create_block
from test_framework.comptool import TestInstance, TestManager
from test_framework.script import CScript, OP_1NEGATE, OP_CHECKSEQUENCEVERIFY, OP_DROP

class BIP9SoftForksTest(ComparisonTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['-whitelist=127.0.0.1']]
        self.setup_clean_chain = True

    def run_test(self):
        self.test = TestManager(self, self.options.tmpdir)
        self.test.add_all_connections(self.nodes)
        NetworkThread().start() # Start up network handling in another thread
        self.test.run()

    def create_transaction(self, node, coinbase, to_address, amount):
        from_txid = node.getblock(coinbase)['tx'][0]
        inputs = [{ "txid" : from_txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = node.createrawtransaction(inputs, outputs)
        tx = CTransaction()
        f = BytesIO(hex_str_to_bytes(rawtx))
        tx.deserialize(f)
        tx.nVersion = 2
        return tx

    def sign_transaction(self, node, tx):
        signresult = node.signrawtransaction(bytes_to_hex_str(tx.serialize()))
        tx = CTransaction()
        f = BytesIO(hex_str_to_bytes(signresult['hex']))
        tx.deserialize(f)
        return tx

    def generate_blocks(self, number, version, test_blocks = []):
        for i in range(number):
            block = create_block(self.tip, create_coinbase(self.height), self.height, self.last_block_time + 1)
            block.nVersion = version
            block.rehash()
            block.solve()
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            self.height += 1
        return test_blocks

    def get_bip9_status(self, key):
        info = self.nodes[0].getblockchaininfo()
        return info['bip9_softforks'][key]

    def test_BIP(self, bipName, activated_version, invalidate, invalidatePostSignature, bitno):
        assert_equal(self.get_bip9_status(bipName)['status'], 'defined')
        assert_equal(self.get_bip9_status(bipName)['since'], 0)

        # generate some coins for later
        self.coinbase_blocks = self.nodes[0].generate(2)
        self.height = 3  # height of the next block to build
        self.tip = int("0x" + self.nodes[0].getbestblockhash(), 0)
        self.nodeaddress = self.nodes[0].getnewaddress()
        self.last_block_time = int(time.time())

        assert_equal(self.get_bip9_status(bipName)['status'], 'defined')
        assert_equal(self.get_bip9_status(bipName)['since'], 0)
        tmpl = self.nodes[0].getblocktemplate({})
        assert(bipName not in tmpl['rules'])
        assert(bipName not in tmpl['vbavailable'])
        assert_equal(tmpl['vbrequired'], 0)
        assert_equal(tmpl['version'], 0x20000000)

        # Test 1
        # Advance from DEFINED to STARTED
        test_blocks = self.generate_blocks(841, 4)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['status'], 'started')
        assert_equal(self.get_bip9_status(bipName)['since'], 144)
        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 0)
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 0)
        tmpl = self.nodes[0].getblocktemplate({})
        assert(bipName not in tmpl['rules'])
        assert_equal(tmpl['vbavailable'][bipName], bitno)
        assert_equal(tmpl['vbrequired'], 0)
        assert(tmpl['version'] & activated_version)

        # Test 1-A
        # check stats after max number of "signalling not" blocks such that LOCKED_IN still possible this period
        test_blocks = self.generate_blocks(211, 4, test_blocks) # 0x00000004 (signalling not)    # 844-633
        test_blocks = self.generate_blocks(10, activated_version) # 0x20000001 (signalling ready)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 221)   #211+10
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 10)
        assert_equal(self.get_bip9_status(bipName)['statistics']['possible'], True)

        # Test 1-B
        # check stats after one additional "signalling not" block --  LOCKED_IN no longer possible this period
        test_blocks = self.generate_blocks(1, 4, test_blocks) # 0x00000004 (signalling not)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 222)   #96+1
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 10)
        assert_equal(self.get_bip9_status(bipName)['statistics']['possible'], False)

        # Test 1-C
        # finish period with "ready" blocks, but soft fork will still fail to advance to LOCKED_IN
        test_blocks = self.generate_blocks(622, activated_version) # 0x20000001 (signalling ready)  #844-222
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 0)
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 0)
        assert_equal(self.get_bip9_status(bipName)['statistics']['possible'], True)
        assert_equal(self.get_bip9_status(bipName)['status'], 'started')

        # Test 2
        # Fail to achieve LOCKED_IN 600 out of 844 signal bit 1, 244 signalling not - failed
        # using a variety of bits to simulate multiple parallel softforks
        test_blocks = self.generate_blocks(300, activated_version) # 0x20000001 (signalling ready)
        test_blocks = self.generate_blocks(122, 4, test_blocks) # 0x00000004 (signalling not)
        test_blocks = self.generate_blocks(300, activated_version, test_blocks) # 0x20000101 (signalling ready)
        test_blocks = self.generate_blocks(122, 4, test_blocks) # 0x20010000 (signalling not)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['status'], 'started')
        assert_equal(self.get_bip9_status(bipName)['since'], 844)
        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 0)
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 0)
        tmpl = self.nodes[0].getblocktemplate({})
        assert(bipName not in tmpl['rules'])
        assert_equal(tmpl['vbavailable'][bipName], bitno)
        assert_equal(tmpl['vbrequired'], 0)
        assert(tmpl['version'] & activated_version)

        # Test 3
        # 633 out of 844 signal bit 1 to achieve LOCKED_IN     633=333+300, 210=100+110
        # using a variety of bits to simulate multiple parallel softforks
        test_blocks = self.generate_blocks(333, activated_version) # 0x20000001 (signalling ready)
        test_blocks = self.generate_blocks(100, 4, test_blocks)    # 0x00000004 (signalling not)
        test_blocks = self.generate_blocks(300, activated_version, test_blocks) # 0x20000101 (signalling ready)
        test_blocks = self.generate_blocks(110, 4, test_blocks)    # 0x20010000 (signalling not)
        yield TestInstance(test_blocks, sync_every_block=False)

        # check counting stats and "possible" flag before last block of this period achieves LOCKED_IN...
        assert_equal(self.get_bip9_status(bipName)['statistics']['elapsed'], 843)
        assert_equal(self.get_bip9_status(bipName)['statistics']['count'], 633)     
        assert_equal(self.get_bip9_status(bipName)['statistics']['possible'], True)
        assert_equal(self.get_bip9_status(bipName)['status'], 'started')

        # ...continue with Test 3
        test_blocks = self.generate_blocks(1, activated_version) # 0x20000001 (signalling ready)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['status'], 'locked_in')
        assert_equal(self.get_bip9_status(bipName)['since'], 3376)     # 844*4
        tmpl = self.nodes[0].getblocktemplate({})
        assert(bipName not in tmpl['rules'])

        # Test 4
        # 843 more version 536870913 blocks (waiting period-1)
        test_blocks = self.generate_blocks(843, 4)
        yield TestInstance(test_blocks, sync_every_block=False)

        assert_equal(self.get_bip9_status(bipName)['status'], 'locked_in')
        assert_equal(self.get_bip9_status(bipName)['since'], 3376)   #844*4
        tmpl = self.nodes[0].getblocktemplate({})
        assert(bipName not in tmpl['rules'])

        # Test 5
        # Check that the new rule is enforced
        spendtx = self.create_transaction(self.nodes[0],
                self.coinbase_blocks[0], self.nodeaddress, 1.0)
        invalidate(spendtx)
        spendtx = self.sign_transaction(self.nodes[0], spendtx)
        spendtx.rehash()
        invalidatePostSignature(spendtx)
        spendtx.rehash()
        block = create_block(self.tip, create_coinbase(self.height), self.height, self.last_block_time + 1)
        block.nVersion = activated_version
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()

        self.last_block_time += 1
        self.tip = block.sha256
        self.height += 1
        yield TestInstance([[block, True]])

        assert_equal(self.get_bip9_status(bipName)['status'], 'active')
        assert_equal(self.get_bip9_status(bipName)['since'], 4220)      #844*5
        tmpl = self.nodes[0].getblocktemplate({})
        assert(bipName in tmpl['rules'])
        assert(bipName not in tmpl['vbavailable'])
        assert_equal(tmpl['vbrequired'], 0)
        assert(not (tmpl['version'] & (1 << bitno)))

        # Test 6
        # Check that the new sequence lock rules are enforced
        spendtx = self.create_transaction(self.nodes[0],
                self.coinbase_blocks[1], self.nodeaddress, 1.0)
        invalidate(spendtx)
        spendtx = self.sign_transaction(self.nodes[0], spendtx)
        spendtx.rehash()
        invalidatePostSignature(spendtx)
        spendtx.rehash()

        block = create_block(self.tip, create_coinbase(self.height), self.height, self.last_block_time + 1)
        block.nVersion = 5
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        self.last_block_time += 1
        yield TestInstance([[block, False]])

        # Restart all
        self.test.clear_all_connections()
        self.stop_nodes()
        self.nodes = []
        shutil.rmtree(self.options.tmpdir + "/node0")
        self.setup_chain()
        self.setup_network()
        self.test.add_all_connections(self.nodes)
        NetworkThread().start()
        self.test.test_nodes[0].wait_for_verack()

    def get_tests(self):
        for test in itertools.chain(
                self.test_BIP('csv', 0x20000001, self.sequence_lock_invalidate, self.donothing, 0),
                self.test_BIP('csv', 0x20000001, self.mtp_invalidate, self.donothing, 0),
                self.test_BIP('csv', 0x20000001, self.donothing, self.csv_invalidate, 0)
        ):
            yield test

    def donothing(self, tx):
        return

    def csv_invalidate(self, tx):
        """Modify the signature in vin 0 of the tx to fail CSV
        Prepends -1 CSV DROP in the scriptSig itself.
        """
        tx.vin[0].scriptSig = CScript([OP_1NEGATE, OP_CHECKSEQUENCEVERIFY, OP_DROP] +
                                      list(CScript(tx.vin[0].scriptSig)))

    def sequence_lock_invalidate(self, tx):
        """Modify the nSequence to make it fails once sequence lock rule is
        activated (high timespan).
        """
        tx.vin[0].nSequence = 0x00FFFFFF
        tx.nLockTime = 0

    def mtp_invalidate(self, tx):
        """Modify the nLockTime to make it fails once MTP rule is activated."""
        # Disable Sequence lock, Activate nLockTime
        tx.vin[0].nSequence = 0x90FFFFFF
        tx.nLockTime = self.last_block_time

if __name__ == '__main__':
    BIP9SoftForksTest().main()
