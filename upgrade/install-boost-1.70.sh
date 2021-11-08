sudo apt-get update

# to uninstall deb version
sudo apt-get -y --purge remove libboost-all-dev libboost-doc libboost-dev
sudo apt autoremove -y

# to uninstall the version which we installed from source
sudo rm -f /usr/lib/libboost_*

# install other dependencies if they are not met
sudo apt-get -y install build-essential python-dev autotools-dev libicu-dev libbz2-dev

# go to home folder
cd
wget http://downloads.sourceforge.net/project/boost/boost/1.70.0/boost_1_70_0.tar.gz
tar -xvf boost_1_70_0.tar.gz
cd boost_1_70_0

# get the no of cpucores to make faster
# cpuCores=`cat /proc/cpuinfo | grep "cpu cores" | uniq | awk '{print $NF}'`
# echo "Available CPU cores: "$cpuCores
./bootstrap.sh  # this will generate ./b2
sudo ./b2 --with=all install
sudo ldconfig

# let's check the installed version
cat /usr/local/include/boost/version.hpp | grep "BOOST_LIB_VERSION"

# clean up
cd ..
sudo rm boost_1_70_0.tar.gz
sudo rm -rf boost_1_70_0

