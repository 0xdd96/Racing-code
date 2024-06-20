#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../mruby
make clean

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$AFL_DIR/afl-clang-fast CFLAGS="-g -O0 -fPIC -fsanitize=memory"  LDFLAGS="-fsanitize=memory -fPIC -lm" LD=$CC ./minirake  $PWD/bin/mruby
mv $PWD/bin/mruby $EVAL_DIR/mruby_fuzz.aurora
