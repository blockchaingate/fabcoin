How to setup a fabcoin test environment in your windows 10 environment.

# Download and unzip.

Download fabcoin-win10.zip , and unzip it to c:/workspace/fabcoin

#configure
Config file is under c:/workspace/fabcoin/data/fabcoin.conf 

Testnet

    testnet=1 
    addnode=35.182.160.212
    addnode=13.59.134.49
    gen=1

MainNet

    addnode=54.215.244.48
    addnode=18.130.8.117
    gen=1
 

# Run wallet program.
Run fabcoin-qt.exe , which is under c:/workspace/fabcoin/bin.
Mostly, windows will warn you because you run this binary code which download from website, and you need allow fabcoin-qt run and access internet to make fabcoin-qt run.

# How to use fabcoin-qt

please refer fabcoin-qt use guide.
