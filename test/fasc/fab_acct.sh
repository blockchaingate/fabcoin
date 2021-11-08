#!/bin/bash

fab_acct() {
  local nodenum="$1"
  port_rpc=$(($RPC_PORT + $nodenum ))
  TR=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} listaccounts`
  echo "node: " $nodenum " account: " $TR 
}

