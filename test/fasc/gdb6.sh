#!/bin/bash

rm -rf $HOME/regtest/6
mkdir $HOME/regtest/6
gdb -args ../../src/fabcoind -server -listen -port=15006 -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcport=14006 -datadir=$HOME/regtest/6/ -connect=localhost:15001 -regtest -pid=$HOME/regtest/6/.pid  -addnode=localhost:15002 -addnode=localhost:15003 -addnode=localhost:15004 -addnode=localhost:15005  -gen

