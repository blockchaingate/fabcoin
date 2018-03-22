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
assert_eq "1" "$BLOCK_NUM" "failed: block # is not 1 !"
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

port_rpc=$(($RPC_PORT + 2 ))
export SC_2_1=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} createcontract "60606040525b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff02191690836c010000000000000000000000009081020402179055506103786001600050819055505b600c80605b6000396000f360606040526008565b600256" 6000000 0.0000004 $ADDR_1 true`
echo $SC_2_1

port_rpc=$(($RPC_PORT + 3 ))
export TXID_3_4=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_4} 10`

echo "TXID (node 3-->4) 10 coin" $TXID_3_4

fab_mining $DAEMON_NUM $FAB_ROOT $RPC_PORT 5

fab_bal $DAEMON_NUM 

#### END Case : create contract #######################
