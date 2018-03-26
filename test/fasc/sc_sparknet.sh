#!/bin/bash
source sc_bash.sh


echo
echo
echo "Test: create contract =================================================="

fab_bal $DAEMON_NUM 

port_rpc=$(($RPC_PORT + 4 ))
export TXID_4_1=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_1} 10`
echo "TXID (node 4-->1) 10 coin" $TXID_4_1

port_rpc=$(($RPC_PORT + 3 ))
export TXID_3_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_4} 10`
echo "TXID (node 3-->4) 10 coin" $TXID_3_4
echo "Above transaction is for node 4 creating contract"

sleep 2
fab_mining_one 5 $FAB_ROOT $RPC_PORT 5 
sleep 5

fab_bal $DAEMON_NUM 

port_rpc=$(($RPC_PORT + 4 ))
  
export SC_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} createcontract  6060604052341561000f57600080fd5b6001600055610245806100236000396000f3006060604052600436106100775763ffffffff7c01000000000000000000000000000000000000000000000000000000006000350416633450bd6a811461007c5780633ccfd60b146100a15780633fb5c1cb146100b657806394e8767d146100cc5780639f2c436f146100e2578063d0e30db0146100f5575b600080fd5b341561008757600080fd5b61008f6100fd565b60405190815260200160405180910390f35b34156100ac57600080fd5b6100b4610103565b005b34156100c157600080fd5b6100b460043561015c565b34156100d757600080fd5b61008f600435610161565b34156100ed57600080fd5b6100b46101db565b6100b461015a565b60005490565b3373ffffffffffffffffffffffffffffffffffffffff166108fc3073ffffffffffffffffffffffffffffffffffffffff16319081150290604051600060405180830381858888f19350505050151561015a57600080fd5b565b600055565b600081151561019157507f30000000000000000000000000000000000000000000000000000000000000006101d6565b60008211156101d65761010081049050600a82066030017f01000000000000000000000000000000000000000000000000000000000000000217600a82049150610191565b919050565b6101e6600054610161565b6040517f73746f7265644e756d62657200000000000000000000000000000000000000008152600c0160405180910390a15600a165627a7a72305820437413a4fecb687d1adadaa9b84c54862b0e1f7a6e831cbe2362704222a577220029 2500000 0.000001 `

export ADDR_SC_4=`echo ${SC_4}|jq '.address'`
echo "node 4 create contract..."
echo $SC_4
echo
echo "Contract address is " $ADDR_SC_4

sleep 5
fab_mining_one 5 $FAB_ROOT $RPC_PORT 25 
sleep 5

fab_bal $DAEMON_NUM 


#### END Case : create contract #######################

echo
echo
echo "Test: call contract =================================================="

port_rpc=$(($RPC_PORT + 1 ))
export SC_LIST_CTR=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} listcontracts`
echo "list contracts return..." 
echo $SC_LIST_CTR

echo
echo
port_rpc=$(($RPC_PORT + 1 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?getaccountinfo?"|sed -e "s?PARAMLIST?${ADDR_SC_4}?" > tmp_get_account.sr
echo "contract account info return..." 
bash tmp_get_account.sr
sleep 2 


echo
echo
port_rpc=$(($RPC_PORT + 1 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 3450bd6a?" > tmp_call_contract.sr
echo "node 1 call contact created at node 4 "
bash tmp_call_contract.sr
sleep 2

#### END Case : call contract #######################

echo
echo

echo "Test: send to contract =================================================="
fab_bal $DAEMON_NUM 
echo

echo "contract address is " ${ADDR_SC_4}
port_rpc=$(($RPC_PORT + 4 ))

# setNumber to 123456
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?sendtocontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 3fb5c1cb000000000000000000000000000000000000000000000000000000000001e240?" > tmp_send_contract.sr
echo "node 4 send to contact created by node 4 "
bash tmp_send_contract.sr
sleep 1

fab_mining_one 5 $FAB_ROOT $RPC_PORT 25 
sleep 2

fab_bal $DAEMON_NUM 
echo

port_rpc=$(($RPC_PORT + 2 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 3450bd6a?" > tmp_call_contract.sr
echo "node 2 call contact created at node 4 "
bash tmp_call_contract.sr

fab_bal $DAEMON_NUM 
echo
fab_mining_one 5 $FAB_ROOT $RPC_PORT 25 
sleep 2
fab_bal $DAEMON_NUM 
#### END Case : send to contract #######################

