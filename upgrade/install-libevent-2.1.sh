cd
wget https://github.com/libevent/libevent/releases/download/release-2.1.10-stable/libevent-2.1.10-stable.tar.gz

tar -xvf libevent-2.1.10-stable.tar.gz
cd libevent-2.1.10-stable
./autogen.sh
./configure
sudo make install
cd
rm libevent-2.1.10-stable.tar.gz
rm -rf libevent-2.1.10-stable


 
