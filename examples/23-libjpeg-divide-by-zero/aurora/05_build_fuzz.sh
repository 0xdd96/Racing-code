#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../libjpeg-turbo-2.0.90
rm build -r
mkdir build
cd build

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$AFL_DIR/afl-clang-fast CXX=$AFL_DIR/afl-clang-fast++ CFLAGS="-g -O0 -fsanitize=address" CXXFLAGS=$CFLAGS cmake -G "Unix Makefiles" -DBUILD_SHARED_LIBS=off ..
make

mv $PWD/cjpeg $EVAL_DIR/cjpeg_fuzz.aurora
