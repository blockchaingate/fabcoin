#!/bin/bash

declare -a arr=("sc_tx.sh" "sc_greet.sh")

for i in "${arr[@]}"
do
   echo "CASE ------"  "$i"
   bash $i
done
