#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../screen-4.7.0
make clean

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CFLAGS="-g -O0" ./configure --disable-shared
CC=$AFL_DIR/afl-clang-fast CFLAGS="-g -O0" ./configure --enable-rxvt_osc --disable-shared
make

mv $PWD/screen $EVAL_DIR/screen_fuzz.aurora