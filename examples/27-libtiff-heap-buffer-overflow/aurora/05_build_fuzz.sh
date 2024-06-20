#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../tiff-4.0.6
make clean

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$AFL_DIR/afl-clang-fast LD=$CC CFLAGS="-g -O0 -fsanitize=address" ./configure --disable-shared
make

mv $PWD/tools/tiffcp $EVAL_DIR/tiffcrop_fuzz.aurora
