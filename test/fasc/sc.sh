#!/bin/bash
source "./assert.sh"
source "./fab_env.sh"
source "./fab_daemon.sh"
source "./fab_mining.sh"
source "./fab_info.sh"
source "./fab_acct.sh"
source "./fab_bal.sh"

DAEMON_NUM=5

FAB_PORT=15000
RPC_PORT=14000

test_path=$(pwd)
FAB_ROOT=${test_path}/../..

assert_eq "CODE" "TEST" "Test is quality!"

fab_env $DAEMON_NUM 
fab_daemon $DAEMON_NUM $FAB_ROOT $FAB_PORT $RPC_PORT

sleep 10 

echo
echo
echo "Test: getinfo =================================================="
fab_info 1
BLOCK_NUM=$?
assert_eq "21" "$BLOCK_NUM" "failed: block # is not 1 !"
echo "Pass: getinfo "
#### END Case : getinf #######################

echo
echo
echo "Test: generate ..."

fab_mining $DAEMON_NUM $FAB_ROOT $RPC_PORT 6

#### END Case : generate #######################

echo
echo
echo "Test: account address =================================================="

port_rpc=$(($RPC_PORT + 1 ))
export ADDR_1=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 1 " ${ADDR_1}

port_rpc=$(($RPC_PORT + 2 ))
export ADDR_2=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 2 " ${ADDR_2}

port_rpc=$(($RPC_PORT + 3 ))
export ADDR_3=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 3 " ${ADDR_3}

port_rpc=$(($RPC_PORT + 4 ))
export ADDR_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 4 " ${ADDR_4}


#### END Case : account address #######################

echo
echo
echo "Test: transaction =================================================="

fab_bal $DAEMON_NUM 

port_rpc=$(($RPC_PORT + 1 ))
export TXID_1_2=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_2} 10`

echo "TXID (node 1-->2) 10 coin" $TXID_1_2

fab_mining $DAEMON_NUM $FAB_ROOT $RPC_PORT 5

fab_bal $DAEMON_NUM 

#### END Case : transaction #######################

echo
echo
echo "Test: create contract =================================================="

fab_bal $DAEMON_NUM 

port_rpc=$(($RPC_PORT + 3 ))
export TXID_3_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_4} 10`
echo "TXID (node 3-->4) 10 coin" $TXID_3_4
echo "Above transaction is for node 4 creating contract"

sleep 2
fab_mining_one 1 $FAB_ROOT $RPC_PORT 5 
sleep 5

port_rpc=$(($RPC_PORT + 4 ))
  
export SC_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} createcontract 6060604052341561000f57600080fd5b6040516103173803806103178339810160405280805160008054600160a060020a03191633600160a060020a03161790559190910190506001818051610059929160200190610060565b50506100fb565b828054600181600116156101000203166002900490600052602060002090601f016020900481019282601f106100a157805160ff19168380011785556100ce565b828001600101855582156100ce579182015b828111156100ce5782518255916020019190600101906100b3565b506100da9291506100de565b5090565b6100f891905b808211156100da57600081556001016100e4565b90565b61020d8061010a6000396000f300606060405263ffffffff7c010000000000000000000000000000000000000000000000000000000060003504166341c0e1b58114610047578063cfae32171461005c57600080fd5b341561005257600080fd5b61005a6100e6565b005b341561006757600080fd5b61006f610127565b60405160208082528190810183818151815260200191508051906020019080838360005b838110156100ab578082015183820152602001610093565b50505050905090810190601f1680156100d85780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b6000543373ffffffffffffffffffffffffffffffffffffffff908116911614156101255760005473ffffffffffffffffffffffffffffffffffffffff16ff5b565b61012f6101cf565b60018054600181600116156101000203166002900480601f0160208091040260200160405190810160405280929190818152602001828054600181600116156101000203166002900480156101c55780601f1061019a576101008083540402835291602001916101c5565b820191906000526020600020905b8154815290600101906020018083116101a857829003601f168201915b5050505050905090565b602060405190810160405260008152905600a165627a7a723058209a62630a1678b0014fdfe901ed4f21cd251e9b7863cfccbf79b1870bcc2e1de100290000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000000c48656c6c6f20576f726c64210000000000000000000000000000000000000000 2500000 0.00000049 ${ADDR_4} true` 
export ADDR_SC_4=`echo ${SC_4}|jq '.address'`
echo "node 4 create contract..."
echo $SC_4
echo
echo "Contract address is " $ADDR_SC_4


fab_mining $DAEMON_NUM $FAB_ROOT $RPC_PORT 5

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
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?callcontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} cfae3217?" > tmp_call_contract.sr
echo "node 1 call contact created at node 4 "
bash tmp_call_contract.sr
sleep 2

#### END Case : call contract #######################

echo
echo
echo "Test: send to contract =================================================="
fab_bal $DAEMON_NUM 
echo
port_rpc=$(($RPC_PORT + 3 ))
export TXID_3_4_S=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_4} 5`
echo "TXID (node 3-->4) 5 coin" $TXID_3_4_S
echo "Above transaction is for node 4 sending to contract"

sleep 2
#fab_mining_one 5 $FAB_ROOT $RPC_PORT 5 
fab_mining $DAEMON_NUM $FAB_ROOT $RPC_PORT 5
sleep 5

echo "node 4 address is " ${ADDR_4}
port_rpc=$(($RPC_PORT + 4 ))
export LIST_UNSPENT_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} listunspent 1 9999999 [\"${ADDR_4}\"]`
echo "node 4 unspent..." 
echo $LIST_UNSPENT_4
echo

echo "contract address is " ${ADDR_SC_4}
port_rpc=$(($RPC_PORT + 4 ))
cat client_command.template|sed -e "s?FAB_ROOT?${FAB_ROOT}?"|sed -e "s?PORT_RPC?${port_rpc}?"|sed -e "s?CLIENTCOMMAND?sendtocontract?"|sed -e "s?PARAMLIST?${ADDR_SC_4} 41c0e1b5 0 250000 0.00000049 ${ADDR_4}?" > tmp_send_contract.sr
echo "node 4 send to contact created by node 4 "
bash tmp_send_contract.sr
sleep 1
echo
fab_mining $DAEMON_NUM $FAB_ROOT $RPC_PORT 5
fab_bal $DAEMON_NUM 
#### END Case : send to contract #######################

