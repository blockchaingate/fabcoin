#!/bin/bash

fab_env() {
  local daemon_num="$1"

  rm -fr $HOME/regtest
  mkdir $HOME/regtest
  for i in `seq 1 ${daemon_num}`;
    do
       mkdir $HOME/regtest/${i}/
    done    
}

