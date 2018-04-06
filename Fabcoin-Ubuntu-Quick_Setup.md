
How to setup a fabcoin test environment in your Ubuntu 16.04 environment.


#  Install Ubuntu 16.04  and  support package
Please refer doc/Build-unix.md to setup Ubuntu 16.04 and install support package.

## Install Ubuntu 16.04 
wallet need GUI desktop.

## Install other support package
Please refer doc/build-unix.md, to install support package for fabcoin.

Below is a quick shell-script to install package.

    sudo apt-get update
    sudo apt-get install git

    sudo apt-get install build-essential 
    sudo apt-get install autoconf libboost-all-dev libssl-dev libprotobuf-dev protobuf-compiler libqt4-dev libqrencode-dev libtool libevent-dev 
    sudo apt-get install python3-zmq libsodium-dev

    cd ~;
    wget http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz
    tar -xvf db-4.8.30.NC.tar.gz
    cd db-4.8.30.NC/build_unix
    mkdir -p build 
    BDB_PREFIX=$(pwd)/build
    ../dist/configure --disable-shared --enable-cxx --with-pic --prefix=$BDB_PREFIX
    make install
    cd ../..

## Install graphic card driver and OpenCL
Please refer doc/GPU-Mining.md and your graphic card specification to install proper drivers and OpenCL .


# Download Fabcoin and configure
    
## Download
Download Fabcoin Ubuntu 16.04 version from  fabcoin.pro/runtime.html, and extract it to $HOME/fabcoin 

## Configure Fabcoin

Set $HOME/.fabcoin as default data folder, config file is $HOME/.fabcoin/data/fabcoin.conf 
Please set below content in fabcoin.conf, to specific testnet.

### Mainnet
    addnode=54.215.244.48
    addnode=18.130.8.117
    gen=1
    G=1                  
    allgpu=1     
    
### Testnet
    testnet=1                                 
    addnode=35.182.160.212
    addnode=13.59.134.49
    gen=1
    G=1
    allgpu=1

# Run 

## Fabcoin full node server 
Run $HOME/fabcoin/bin/fabcoind will start fabcoin full node server. 
Run fabcoind -h , will show all the running options.

run fabcoin full node in background and start all GPU device mining.

     $HOME/fabcoin/bin/fabcoind -daemon  -addnode=54.215.244.48 -addnode=18.130.8.117 -gen -G -allgpu

check log
    cd $HOME/.fabcoin
    vi debug.log

## Run wallet program. 

Will start wallet program and also start GPU device mining

     $HOME/fabcoin/bin/fabcoin-qt   -addnode=54.215.244.48 -addnode=18.130.8.117 -gen -G -allgpu
    
## How to use  

Please refer fabcoin website and User Guide and document under doc.
