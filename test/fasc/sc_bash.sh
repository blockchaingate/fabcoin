#!/bin/bash
source "./assert.sh"
source "./fab_env.sh"
source "./fab_daemon.sh"
source "./fab_mining.sh"
source "./fab_info.sh"
source "./fab_acct.sh"
source "./fab_bal.sh"
source "./fab_blockheight.sh"

export DAEMON_NUM=5

export FAB_PORT=15000
export RPC_PORT=14000

test_path=$(pwd)
export FAB_ROOT=${test_path}/../..

assert_eq "CODE" "TEST" "Test is quality!"

fab_env $DAEMON_NUM 
fab_daemon $DAEMON_NUM $FAB_ROOT $FAB_PORT $RPC_PORT

sleep 10 

echo
echo
echo "Initial mining  =================================================="
fab_info 1
BLOCK_NUM=$?

assert_eq "21" "$BLOCK_NUM" "failed: block # is not 1 !"

fab_mining $DAEMON_NUM $FAB_ROOT $RPC_PORT 10

echo
fab_info 1

echo
echo "Set account address =================================================="

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

port_rpc=$(($RPC_PORT + 5 ))
export ADDR_5=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 5 " ${ADDR_5}

echo
echo
echo "get VM contract address =================================================="

port_rpc=$(($RPC_PORT + 1 ))
export VMADDR_1=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${ADDR_1}`
export VMADDR_1=`echo ${VMADDR_1}|jq -r '.addressinvm'`
echo "VM address node 1 " ${VMADDR_1}

port_rpc=$(($RPC_PORT + 2 ))
export VMADDR_2=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${ADDR_2}`
export VMADDR_2=`echo ${VMADDR_2}|jq -r '.addressinvm'`
echo "VM address node 2 " ${VMADDR_2}

port_rpc=$(($RPC_PORT + 3 ))
export VMADDR_3=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${ADDR_3}`
export VMADDR_3=`echo ${VMADDR_3}|jq -r '.addressinvm'`
echo "VM address node 3 " ${VMADDR_3}

port_rpc=$(($RPC_PORT + 4 ))
export VMADDR_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${ADDR_4}`
export VMADDR_4=`echo ${VMADDR_4}|jq -r '.addressinvm'`
echo "VM address node 4 " ${VMADDR_4}

port_rpc=$(($RPC_PORT + 5 ))
export VMADDR_5=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getvmaddress ${ADDR_5}`
export VMADDR_5=`echo ${VMADDR_5}|jq -r '.addressinvm'`
echo "VM address node 5 " ${VMADDR_5}

echo



#### END: Set  account address #######################

