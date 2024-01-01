#/bin/bash

EVAL_DIR=`pwd`

# download libzip
wget https://libzip.org/download/libzip-1.2.0.tar.gz
tar -xzf libzip-1.2.0.tar.gz
cd libzip-1.2.0

# build normal version for tracing
CFLAGS="-g -O0" ./configure --disable-shared
make

mv $PWD/src/ziptool $EVAL_DIR/ziptool_trace
