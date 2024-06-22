#/bin/bash

EVAL_DIR=`pwd`

# download libjpeg
wget https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/2.0.90.tar.gz
tar -xzf 2.0.90.tar.gz
cd libjpeg-turbo-2.0.90

# build normal version for tracing
mkdir build
cd build
CFLAGS="-g -O0" CXXFLAGS=$CFLAGS cmake -G "Unix Makefiles" -DBUILD_SHARED_LIBS=off ..
make

mv $PWD/cjpeg $EVAL_DIR/cjpeg_trace
