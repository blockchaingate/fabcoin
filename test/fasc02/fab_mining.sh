#!/bin/bash

fab_mining() {
  local DAEMON_NUM="$1"
  local FAB_ROOT="$2"
  local RPC_PORT="$3"
  local round_num=$4

  echo
  COUNTER=0
  echo "Mining " $round_num "X 5 blocks"
  while [  $COUNTER -lt $round_num ]; do
    let COUNTER=COUNTER+1
    for j in `seq 1 ${DAEMON_NUM}`;
    do
      port_rpc=$(( $RPC_PORT + $j ))
      TR=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} generate 1`
      sleep 1
  
     if [  "$COUNTER" == "$round_num" ]; then 
        port_rpc=$(($RPC_PORT + $j))
        TR=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getinfo`
        BLOCK_NUM=`echo $TR|jq '.blocks'`
        BALANCE_NUM=`echo $TR|jq '.balance'`
        echo "node: " $j "   block #: " $BLOCK_NUM "   balance=" $BALANCE_NUM
      fi
    done
    sleep 1
  done
  echo
}

fab_mining_one() {
  local node_num=$1
  local FAB_ROOT="$2"
  local RPC_PORT="$3"
  local round_num=$4

  echo
  COUNTER=0
  echo "Mining by node " $node_num " with "  $round_num " blocks"
  port_rpc=$(( $RPC_PORT + $node_num ))
  TR=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} generate ${round_num}`
  sleep 1
  
   TR=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getinfo`
   BLOCK_NUM=`echo $TR|jq '.blocks'`
   BALANCE_NUM=`echo $TR|jq '.balance'`
   echo "node: " $node_num "   block #: " $BLOCK_NUM "   balance=" $BALANCE_NUM
  echo
}

