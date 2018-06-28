#!/bin/bash

fab_info() {
  local nodenum="$1"
  port_rpc=$(($RPC_PORT + $nodenum ))
  TR=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getinfo`
  BLOCK_NUM=`echo $TR|jq '.blocks'`
  BALANCE_NUM=`echo $TR|jq '.balance'`
  echo "node is " $nodenum "   blocks=" $BLOCK_NUM "   balance=" $BALANCE_NUM
  echo
  return $BLOCK_NUM 
}

