#!/bin/bash
source sc_bash.sh

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

