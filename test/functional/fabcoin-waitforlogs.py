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


RPC_INVALID_PARAMETER = -8

class FabcoinRPCWaitforlogs(FabcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-logevents=1"]]

    def create_contracts_with_logs(self):
        contract_addresses = []
        send_result = []
        block_hashes = []

        self.nodes[0].generate(COINBASE_MATURITY+100)
        contract_address = self.nodes[0].createcontract("6060604052600d600055341561001457600080fd5b61017e806100236000396000f30060606040526004361061004c576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff168063027c1aaf1461004e5780635b9af12b14610058575b005b61005661008f565b005b341561006357600080fd5b61007960048080359060200190919050506100a1565b6040518082815260200191505060405180910390f35b60026000808282540292505081905550565b60007fc5c442325655248f6bccf5c6181738f8755524172cea2a8bd1e38e43f833e7f282600054016000548460405180848152602001838152602001828152602001935050505060405180910390a17fc5c442325655248f6bccf5c6181738f8755524172cea2a8bd1e38e43f833e7f282600054016000548460405180848152602001838152602001828152602001935050505060405180910390a1816000540160008190555060005490509190505600a165627a7a7230582015732bfa66bdede47ecc05446bf4c1e8ed047efac25478cb13b795887df70f290029")['address']
        self.nodes[0].generate(1)
        contract_addresses.append(contract_address)
        send_result.append(self.nodes[0].sendtocontract(contract_address,"5b9af12b"))
        block_hashes.append(self.nodes[0].generate(1))

        contract_address = self.nodes[0].createcontract("6060604052341561000f57600080fd5b61029b8061001e6000396000f300606060405260043610610062576000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806394e8767d14610067578063b717cfe6146100a6578063d3b57be9146100bb578063f7e52d58146100d0575b600080fd5b341561007257600080fd5b61008860048080359060200190919050506100e5565b60405180826000191660001916815260200191505060405180910390f35b34156100b157600080fd5b6100b961018e565b005b34156100c657600080fd5b6100ce6101a9565b005b34156100db57600080fd5b6100e36101b3565b005b600080821415610117577f30000000000000000000000000000000000000000000000000000000000000009050610186565b5b600082111561018557610100816001900481151561013257fe5b0460010290507f01000000000000000000000000000000000000000000000000000000000000006030600a8481151561016757fe5b06010260010281179050600a8281151561017d57fe5b049150610118565b5b809050919050565b60008081548092919060010191905055506101a76101b3565b565b6101b161018e565b565b7f746f7069632034000000000000000000000000000000000000000000000000007f746f7069632033000000000000000000000000000000000000000000000000007f746f7069632032000000000000000000000000000000000000000000000000007f746f70696320310000000000000000000000000000000000000000000000000060405180807f3700000000000000000000000000000000000000000000000000000000000000815250600101905060405180910390a45600a165627a7a72305820262764914338437fc49c9f752503904820534b24092308961bc10cd851985ae50029")['address']      
        self.nodes[0].generate(1)
        contract_addresses.append(contract_address)
        send_result.append(self.nodes[0].sendtocontract(contract_address,"d3b57be9"))

        block_hashes.append(self.nodes[0].generate(1))

        return contract_addresses, send_result, block_hashes


    def check_topics(self, contract_addresses,block_hashes,send_result):
        filters = {}
        address = []
        address.append(contract_addresses[0])
        filters["addresses"] = address

        topic = []
        topic.append("c5c442325655248f6bccf5c6181738f8755524172cea2a8bd1e38e43f833e7f2")
        filters["topics"] = topic


        ret = self.nodes[0].waitforlogs(COINBASE_MATURITY+102,COINBASE_MATURITY+105,filters)

        assert_equal(ret['entries'][0]['blockHash'], block_hashes[0][0])
        assert_equal(ret['entries'][0]['blockNumber'], COINBASE_MATURITY+102)
        assert_equal(ret['entries'][0]['transactionHash'], send_result[0]['txid'])
        assert_equal(ret['entries'][0]['transactionIndex'], 1)
        assert_equal(ret['entries'][0]['from'], send_result[0]['hash160'])
        assert_equal(ret['entries'][0]['to'], contract_addresses[0])
        assert_equal(ret['entries'][0]['gasUsed'], 30991)
        assert_equal(ret['entries'][0]['contractAddress'], contract_addresses[0])
        assert_equal(ret['entries'][0]['topics'], ["c5c442325655248f6bccf5c6181738f8755524172cea2a8bd1e38e43f833e7f2"])
        assert_equal(ret['entries'][0]['data'], "000000000000000000000000000000000000000000000000000000000000000d000000000000000000000000000000000000000000000000000000000000000d0000000000000000000000000000000000000000000000000000000000000000")
        assert_equal(ret['count'], 2)
        assert_equal(ret['nextblock'], COINBASE_MATURITY+106)

        topic.clear()
        topic.append("c5c442325655248f6bccf5c6181738f8755524172cea2a8bd1e38e43f833e7f3")      #error topics

        ret = self.nodes[0].waitforlogs(COINBASE_MATURITY+102,COINBASE_MATURITY+105,filters)
        assert_equal(ret,{"entries":[],"count":0,"nextblock":COINBASE_MATURITY+106})
 
    def check_waitforlogs(self, contract_addresses, send_result, block_hashes):
        self.nodes[0].sendtocontract(contract_addresses[1],"d3b57be9")

        self.nodes[0].generate(10)

        ret = self.nodes[0].waitforlogs(COINBASE_MATURITY+104,COINBASE_MATURITY+104)

        assert_equal(ret['entries'][0]['blockHash'], block_hashes[1][0])
        assert_equal(ret['entries'][0]['blockNumber'], COINBASE_MATURITY+104)
        assert_equal(ret['entries'][0]['transactionHash'], send_result[1]['txid'])
        assert_equal(ret['entries'][0]['transactionIndex'], 1)
        assert_equal(ret['entries'][0]['from'], send_result[1]['hash160'])
        assert_equal(ret['entries'][0]['to'], contract_addresses[1])
        assert_equal(ret['entries'][0]['gasUsed'], 43679)
        assert_equal(ret['entries'][0]['contractAddress'], contract_addresses[1])
        assert_equal(ret['entries'][0]['topics'], ["746f706963203100000000000000000000000000000000000000000000000000","746f706963203200000000000000000000000000000000000000000000000000"
                                                ,"746f706963203300000000000000000000000000000000000000000000000000","746f706963203400000000000000000000000000000000000000000000000000"])
        assert_equal(ret['entries'][0]['data'], "37")
        assert_equal(ret['count'], 1)
        assert_equal(ret['nextblock'], COINBASE_MATURITY+105)

        try:
            self.nodes[0].waitforlogs(0,0)
        except JSONRPCException as exp:
            assert_equal(exp.error["code"], RPC_INVALID_PARAMETER)

        try:
            self.nodes[0].waitforlogs(1,0)
        except JSONRPCException as exp:
            assert_equal(exp.error["code"], RPC_INVALID_PARAMETER)

        try:
            self.nodes[0].waitforlogs(200,100)
        except JSONRPCException as exp:
            assert_equal(exp.error["code"], RPC_INVALID_PARAMETER)

    def run_test(self):
        contract_addresses, send_result, block_hashes = self.create_contracts_with_logs()

        self.check_waitforlogs(contract_addresses, send_result, block_hashes)
        self.check_topics(contract_addresses, block_hashes, send_result)
        self.stop_nodes()
        self.start_nodes()               #start node again
        self.check_topics(contract_addresses, block_hashes,send_result)

if __name__ == '__main__':
    FabcoinRPCWaitforlogs().main()
