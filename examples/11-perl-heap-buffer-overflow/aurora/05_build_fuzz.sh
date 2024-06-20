#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

export AFL_CC=clang-6.0
# build instrument version for fuzzing
cd ../perl5
make clean

./Configure -Dprefix=$PWD/build -Dcc=$AFL_DIR/afl-clang-fast -A ccflags="-g -O0 -fsanitize=address -fPIC" -Dld=$AFL_DIR/afl-clang-fast -A ldflags="-g -O0 -fsanitize=address" -Dusedevel -d -e -s
make install.perl

mv $PWD/build/bin/perl5.31.7 $EVAL_DIR/perl_fuzz.aurora