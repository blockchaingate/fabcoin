#!/bin/bash
source "./fab_mining.sh"

fab_daemon() {
  local daemon_num="$1"
  local fab_root="$2"
  local fab_port=$3
  local rpc_port="$4"

  killall fabcoind
  sleep 3
  rm -fr ~/.fabcoin
  for i in `seq 1 ${daemon_num}`;
    do
      portf=$(($fab_port + $i))
      portr=$(($rpc_port + $i))
      portn=$portf
     if [ "$i" != "$daemon_num" ]; then
       portn=$(($portf + 1))
     else
       portn=$(($fab_port + 1))
     fi

      addnode=" "
      for j in `seq 1 ${daemon_num}`;
      do
        portfn=$(($fab_port + $j))
        if [ "$portfn" != "$portf" ]; then
          addnode+=" -addnode=localhost:${portfn} "
        fi
      done

      ${fab_root}/src/fabcoind -server -listen -port=${portf} -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcport=${portr} -datadir=$HOME/regtest/${i}/ -connect=localhost:${portn} -regtest -pid=$HOME/regtest/${i}/.pid -daemon -debug  ${addnode}  -gen
     if [ "$i" != "1" ]; then
       sleep 1
     else
       sleep 10
       fab_mining_one 1 $FAB_ROOT $RPC_PORT 20
       sleep 3
     fi
    done    
}

