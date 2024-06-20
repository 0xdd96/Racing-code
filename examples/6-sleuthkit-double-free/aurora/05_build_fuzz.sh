#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../sleuthkit
make clean

export AFL_CC=clang-6.0
export AFL_CXX=clang++-6.0
# build instrument version for fuzzing
CC=$AFL_DIR/afl-clang-fast CXX=$AFL_DIR/afl-clang-fast++ LD=$CC CFLAGS="-g -O0 -fPIE" CXXFLAGS=$CFLAGS LDFLAGS=$CFLAGS ./configure --disable-shared
make

mv $PWD/tools/fstools/fls $EVAL_DIR/fls_fuzz.aurora