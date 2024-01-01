#/bin/bash

EVAL_DIR=`pwd`

# download python
#wget https://github.com/python/cpython/archive/refs/tags/v2.7.11.zip
unzip v2.7.11.zip

# build normal version for tracing
cd cpython-2.7.11
mkdir build-trace
cd build-trace
CFLAGS="-g -O0" ../configure --prefix=$PWD --disable-shared
make
make install

cp $PWD/bin/python $EVAL_DIR/python_trace
