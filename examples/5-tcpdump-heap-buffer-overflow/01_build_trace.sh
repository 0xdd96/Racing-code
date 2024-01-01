#/bin/bash

EVAL_DIR=`pwd`

wget https://github.com/the-tcpdump-group/libpcap/archive/refs/tags/libpcap-1.8.1.tar.gz
tar -xzf libpcap-1.8.1.tar.gz
mv libpcap-libpcap-1.8.1 libpcap-1.8.1
cd libpcap-1.8.1
CFLAGS="-g -O0 -fPIC"  ./configure --disable-shared
make

cd $EVAL_DIR
# download tcpdump
wget https://github.com/the-tcpdump-group/tcpdump/archive/refs/tags/tcpdump-4.9.2.tar.gz
tar -xzf tcpdump-4.9.2.tar.gz
mv tcpdump-tcpdump-4.9.2 tcpdump-4.9.2
cd tcpdump-4.9.2

# build normal version for tracing
CFLAGS="-g -O0 -fPIC"  ./configure --disable-shared
make

mv $PWD/tcpdump $EVAL_DIR/tcpdump_trace
