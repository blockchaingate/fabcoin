#!/bin/bash

sudo apt-get update
echo "check gcc version to make sure it is greated than 7..."
gccver=$(gcc -dumpversion|cut -c 1-1)
if [ $gccver -lt 7 ]
then
    echo "upgrading gcc ..."
    sh install-gcc7.sh 
fi

echo "install libgmp-dev ..."
sudo apt-get install libgmp-dev -y 

echo "checking libevent ..."
eventver=$(pkg-config --modversion libevent)
echo "current version is " $eventver
if $(dpkg --compare-versions "$eventver" "lt" "2.1.10")
then
	echo "install libevent 2.1.10..."
	sh install-libevent-2.1.sh
fi

echo "checking libboost ..."
boost_version=$(cat /usr/local/include/boost/version.hpp | grep define | grep "BOOST_VERSION " | cut -d' ' -f3)
boost_version=$(echo "$boost_version / 100000" | bc).$(echo "$boost_version / 100 % 1000" | bc).$(echo "$boost_version % 100 " | bc)
echo "current version is " $boost_version 
if $(dpkg --compare-versions "$boost_version" "lt" "1.70.0")
then
	echo "install boost 1.70..."
	sh install-boost-1.70.sh
fi

echo ""
echo ""
echo "Now you can download fabcoin package and run it." 
echo ""
echo ""



