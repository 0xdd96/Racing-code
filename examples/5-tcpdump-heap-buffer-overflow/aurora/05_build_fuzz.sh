#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../tcpdump-4.9.2
make clean

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$AFL_DIR/afl-clang-fast CFLAGS="-g -O0 -fsanitize=address"  ./configure --disable-shared
make

mv $PWD/tcpdump $EVAL_DIR/tcpdump_fuzz.aurora
