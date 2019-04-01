#!/bin/bash
source sc_bash.sh


echo
echo
echo "Test: create contract =================================================="

fab_bal $DAEMON_NUM 

#port_rpc=$(($RPC_PORT + 4 ))
#export TXID_4_1=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_1} 10`
#echo "TXID (node 4-->1) 10 coin" $TXID_4_1

sleep 12 
port_rpc=$(($RPC_PORT + 2 ))
export TXID_2_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_4} 5`
echo "TXID (node 2-->4 contract account) 5 coin" $TXID_3_4
echo "Above transaction is for node 4 creating contract"

sleep 8
fab_mining_one 5 $FAB_ROOT $RPC_PORT 1 
sleep 8

fab_bal $DAEMON_NUM 

port_rpc=$(($RPC_PORT + 4 ))
  
export SC_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} createcontract  608060405234801561001057600080fd5b50600160a060020a033216600090815260208190526040902061271090556102578061003d6000396000f3006080604052600436106100615763ffffffff7c01000000000000000000000000000000000000000000000000000000006000350416637bd703e8811461006657806390b98a11146100a657806396e4ee3d146100eb578063f8b2cb4f14610106575b600080fd5b34801561007257600080fd5b5061009473ffffffffffffffffffffffffffffffffffffffff60043516610134565b60408051918252519081900360200190f35b3480156100b257600080fd5b506100d773ffffffffffffffffffffffffffffffffffffffff6004351660243561014f565b604080519115158252519081900360200190f35b3480156100f757600080fd5b506100946004356024356101ff565b34801561011257600080fd5b5061009473ffffffffffffffffffffffffffffffffffffffff60043516610203565b600061014961014283610203565b60026101ff565b92915050565b73ffffffffffffffffffffffffffffffffffffffff331660009081526020819052604081205482111561018457506000610149565b73ffffffffffffffffffffffffffffffffffffffff33811660008181526020818152604080832080548890039055938716808352918490208054870190558351868152935191937fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef929081900390910190a350600192915050565b0290565b73ffffffffffffffffffffffffffffffffffffffff16600090815260208190526040902054905600a165627a7a72305820e3121775990e93ede65191e3431bcd51df9fe755ac04330530b72946cbbf63260029 2500000 0.000001 ${ADDR_4}`

export ADDR_SC_4=`echo ${SC_4}|jq '.address'`
echo "node 4 create contract..."
echo $SC_4
echo
echo "Contract address is " $ADDR_SC_4

sleep 5
fab_mining_one 5 $FAB_ROOT $RPC_PORT 1 
sleep 5

fab_bal $DAEMON_NUM 


#### END Case : create contract #######################

echo
echo
echo "Test: call contract =================================================="

echo
echo


echo
echo
port_rpc=$(($RPC_PORT + 1 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} f8b2cb4f000000000000000000000000${VMADDR_4}?" > tmp_call_contract.sr
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
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?sendtocontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 90b98a11000000000000000000000000${VMADDR_3}00000000000000000000000000000000000000000000000000000000000000aa?" > tmp_send_contract.sr
echo "node 4 send to contact to node 3 "
bash tmp_send_contract.sr
sleep 8 

fab_mining_one 5 $FAB_ROOT $RPC_PORT 1 
sleep 8 

port_rpc=$(($RPC_PORT + 1 ))
export TXID_1_3=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_4} 5`
echo "TXID (node 1-->4 contract account) 5 coin" $TXID_1_3
echo "Above transaction is for node 4 sendtocontract -->node 2"
sleep 8
fab_mining_one 5 $FAB_ROOT $RPC_PORT 1 
sleep 8

port_rpc=$(($RPC_PORT + 4 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?sendtocontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 90b98a11000000000000000000000000${VMADDR_2}000000000000000000000000000000000000000000000000000000000000000a 0 2500000 0.000001 ${ADDR_4}?" > tmp_send_contract.sr
echo "node 4 send to contact to node 2 "
bash tmp_send_contract.sr
sleep 8 

fab_mining_one 5 $FAB_ROOT $RPC_PORT 1 
sleep 8 

port_rpc=$(($RPC_PORT + 1 ))
export TXID_1_3=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_3} 5`
echo "TXID (node 1-->3 contract account) 5 coin" $TXID_1_3
echo "Above transaction is for node 3 sendtocontract -->node 1"
sleep 8

port_rpc=$(($RPC_PORT + 3 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?sendtocontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 90b98a11000000000000000000000000${VMADDR_1}0000000000000000000000000000000000000000000000000000000000000005 0 2500000 0.000001 ${ADDR_3}?" > tmp_send_contract.sr
echo "node 3 send to contact to node 1 "
bash tmp_send_contract.sr
sleep 8 

port_rpc=$(($RPC_PORT + 3 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?sendtocontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 90b98a11000000000000000000000000${VMADDR_5}0000000000000000000000000000000000000000000000000000000000000003?" > tmp_send_contract.sr


fab_mining_one 5 $FAB_ROOT $RPC_PORT 1 
sleep 8 


port_rpc=$(($RPC_PORT + 4 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} f8b2cb4f000000000000000000000000${VMADDR_1}?" > tmp_call_contract.sr
echo "node 4 get node 1 token balance "
bash tmp_call_contract.sr|jq '.executionResult'|jq '.output'

port_rpc=$(($RPC_PORT + 4 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} f8b2cb4f000000000000000000000000${VMADDR_2}?" > tmp_call_contract.sr
echo "node 4 get node 2 token balance "
bash tmp_call_contract.sr|jq '.executionResult'|jq '.output'

port_rpc=$(($RPC_PORT + 4 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} f8b2cb4f000000000000000000000000${VMADDR_3}?" > tmp_call_contract.sr
echo "node 4 get node 3 token balance "
bash tmp_call_contract.sr|jq '.executionResult'|jq '.output'

echo
port_rpc=$(($RPC_PORT + 4 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} f8b2cb4f000000000000000000000000${VMADDR_4}?" > tmp_call_contract.sr
echo "node 4 get its token balance "
bash tmp_call_contract.sr|jq '.executionResult'|jq '.output'

sleep 5
fab_mining_one 5 $FAB_ROOT $RPC_PORT 25  >/dev/null 2>&1
sleep 5
fab_bal $DAEMON_NUM 

fab_blockheight $DAEMON_NUM 

#### END Case : send to contract #######################

