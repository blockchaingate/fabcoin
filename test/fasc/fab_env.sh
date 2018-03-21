#!/bin/bash

fab_env() {
  local daemon_num="$1"

  cd $HOME

  rm -fr regtest
  mkdir regtest
  for i in `seq 1 ${daemon_num}`;
    do
       mkdir $HOME/regtest/${i}/
    done    
}

