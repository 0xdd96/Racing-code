#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../ezxml
make clean

export AFL_CC=clang-6.0
# build instrument version for fuzzing
$AFL_DIR/afl-clang-fast -Wall -O0 -DEZXML_TEST -g -ggdb -o ezxml_fuzz ezxml.c

mv $PWD/ezxml_fuzz $EVAL_DIR/ezxml_fuzz.aurora
