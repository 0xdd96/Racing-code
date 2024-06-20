#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../cpython-2.7.11
rm build-rca -rf
mkdir build-rca
cd build-rca

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$AFL_DIR/afl-clang-fast  LD=$CC CFLAGS="-g -O0" ../configure --prefix=$PWD --disable-shared
make
make install

cp $PWD/bin/python $EVAL_DIR/python_fuzz.aurora
