source "./assert.sh"
source "./fab_env.sh"
source "./fab_daemon.sh"
source "./fab_mining.sh"
source "./fab_info.sh"
source "./fab_acct.sh"

DAEMON_NUM=5

FAB_PORT=15000
RPC_PORT=14000

test_path=$(pwd)
FAB_ROOT=${test_path}/../..

assert_eq "CODE" "TEST" "Test is quality!"

fab_env $DAEMON_NUM 
fab_daemon $DAEMON_NUM $FAB_ROOT $FAB_PORT $RPC_PORT

sleep 10 

#### Test Case : getinfo #######################
echo
echo
echo "Test: getinfo ..."
fab_info 1
BLOCK_NUM=$?
assert_eq "1" "$BLOCK_NUM" "failed: block # is not 1 !"
echo "Pass: getinfo "
#### END Case : getinf #######################

#### Test Case : generate #######################
echo
echo
echo "Test: generate ..."

fab_mining $DAEMON_NUM $FAB_ROOT $RPC_PORT 6

echo "Pass: generate "
#### END Case : generate #######################

#### Test Case : account address #######################
echo
echo
echo "Test: account address ..."

port_rpc=$(($RPC_PORT + 1 ))
export ADDR_1=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 1 " ${ADDR_1}

port_rpc=$(($RPC_PORT + 2 ))
export ADDR_2=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 2 " ${ADDR_2}

port_rpc=$(($RPC_PORT + 3 ))
export ADDR_3=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} getaccountaddress ""`
echo "address node 3 " ${ADDR_3}

echo "Pass: account address "
#### END Case : account address #######################

#### Test Case : transaction #######################
echo
echo
echo "Test: transaction ..."

for i in `seq 1 ${DAEMON_NUM}`; 
do
fab_acct $i 
done


port_rpc=$(($RPC_PORT + 1 ))
export TXID_1_2=`${FAB_ROOT}/src/fabcoin-cli -regtest -rpcuser=fabcoinrpc -rpcpassword=P0 -rpcconnect=localhost:${port_rpc} sendtoaddress ${ADDR_2} 10`

echo "TXID (node 1-->2) 10 coin" $TXID_1_2

fab_mining $DAEMON_NUM $FAB_ROOT $RPC_PORT 5

for i in `seq 1 ${DAEMON_NUM}`; 
do
fab_acct $i 
done


echo "Pass:transactions "
#### END Case : transaction #######################
