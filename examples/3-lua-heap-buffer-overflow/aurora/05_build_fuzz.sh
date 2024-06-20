#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../lua-5.0
make clean

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$AFL_DIR/afl-clang-fast  MYCFLAGS="-g -O0 -fsanitize=address" MYLDFLAGS="-fsanitize=address" make -e

mv $PWD/bin/lua $EVAL_DIR/lua_fuzz.aurora