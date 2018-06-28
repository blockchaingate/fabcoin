#!/bin/bash

fab_blockheight() {
echo
echo "Block height now --- "
for i in `seq 1 ${1}`;
do
  local nodenum=$i
  port_rpc=$(($RPC_PORT + $nodenum ))
  TR=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getinfo|jq '.blocks'`
  echo "node: " $nodenum " block height: " $TR

done
echo
}

