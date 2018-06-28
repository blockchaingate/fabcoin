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
  
bytecode=`cat StandardFIP001.bin`
export SC_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} createcontract ${SCADDR_4} ${bytecode} ${ADDR_4} 2500000 0.000001`

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
echo "Test: deposit to =================================================="

echo
echo

sleep 8 
port_rpc=$(($RPC_PORT + 4 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?sendtocontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} b760faf9000000000000000000000000${VMADDR_3} ${ADDR_4} 2500000 0.000001 10 ${ADDR_3}?" > tmp_send_contract.sr
echo "node 4 send to contact to node 3, deposit 10 coins "
bash tmp_send_contract.sr
sleep 8 

fab_mining_one 5 $FAB_ROOT $RPC_PORT 1 
sleep 8 


echo
echo
echo "Test: Withdraw request =================================================="

echo
echo


port_rpc=$(($RPC_PORT + 4 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?sendtocontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 74899a7e000000000000000000000000000000000000000000000000000000001dcd6500 ${ADDR_4}?" > tmp_send_contract.sr
echo "node 4 send to contact, withdraw request 5 coins "
bash tmp_send_contract.sr
sleep 8 

fab_mining_one 5 $FAB_ROOT $RPC_PORT 1 
sleep 8 

echo
echo
echo "Test: Get withdraw address =================================================="

echo
echo


port_rpc=$(($RPC_PORT + 4 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 4c772c060000000000000000000000000000000000000000000000000000000000000001?" > tmp_call_contract.sr
echo "node 4 call to contact, get withdraw address"
bash tmp_call_contract.sr

echo
echo
echo "Test: BEFORE Withdraw balance at sequnce id 1 =================================================="

echo
echo


port_rpc=$(($RPC_PORT + 1 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 1f4985d30000000000000000000000000000000000000000000000000000000000000001?" > tmp_call_contract.sr
echo "BEFORE: node 1 call contact, get withdraw balance on sequence 1 "
bash tmp_call_contract.sr



echo
echo
echo "Test: Withdraw Confirm =================================================="

echo
echo

port_rpc=$(($RPC_PORT + 1 ))
export TXID_1_3=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_3} 5`
echo "TXID (node 1-->3 contract account) 5 coin" $TXID_1_3
echo "Above transaction is for node 3 send to contract"
sleep 8

fab_mining_one 5 $FAB_ROOT $RPC_PORT 1 
sleep 8 

port_rpc=$(($RPC_PORT + 3 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?sendtocontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 267922510000000000000000000000000000000000000000000000000000000000000001 ${ADDR_3} 2500000 0.000001 6 ${ADDR_4}?" > tmp_send_contract.sr
echo "node 3 send to contact, withdraw confirm 6 coins "
bash tmp_send_contract.sr
sleep 8 

fab_mining_one 5 $FAB_ROOT $RPC_PORT 1 
sleep 8 



echo
echo
echo "Test: AFTER Withdraw balance at sequnce id 1 =================================================="

echo
echo


port_rpc=$(($RPC_PORT + 1 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 1f4985d30000000000000000000000000000000000000000000000000000000000000001?" > tmp_call_contract.sr
echo "AFTER: node 1 call contact, get withdraw balance on sequence 1 "
bash tmp_call_contract.sr

echo
echo
echo "Test: Final Status =================================================="

echo
echo
port_rpc=$(($RPC_PORT + 4 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?getcontractinfo?"|sed -e "s?PARAMLIST?${ADDR_SC_4}?" > tmp_get_contract.sr
echo "node 4 get contact created at node 4 "
bash tmp_get_contract.sr

sleep 5
fab_mining_one 5 $FAB_ROOT $RPC_PORT 25  >/dev/null 2>&1
sleep 5
fab_bal $DAEMON_NUM 

fab_blockheight $DAEMON_NUM 

#### END Case : send to contract #######################

