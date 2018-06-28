#!/bin/bash
source "./fab_acct.sh"

fab_bal() {
echo
echo "Balance now --- "
for i in `seq 1 ${1}`;
do
fab_acct $i
done
echo
}

