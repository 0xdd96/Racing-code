#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../libxml2
make clean

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$AFL_DIR/afl-clang-fast CFLAGS="-g -O0" ./configure --disable-shared
make

mv $PWD/xmllint $EVAL_DIR/xmllint_fuzz.aurora