#/bin/bash

EVAL_DIR=`pwd`
RACING_DIR=${RACING_DIR:=/Racing-eval}


if [ -f "$EVAL_DIR/temp_data/inst_id" ]; then
    rm "$EVAL_DIR/temp_data/inst_id"
    rm "$EVAL_DIR/temp_data/bb_id" 
    rm "$EVAL_DIR/temp_data/trace-id.log" 
fi

cd ezxml
make clean

export AFL_CC=clang-6.0
# build instrument version for fuzzing
$RACING_DIR/Racing-code/afl-clang-fast -Wall -O0 -DEZXML_TEST -g -ggdb -poc_trace=$EVAL_DIR/temp_data/poc.addr2line -o ezxml_fuzz ezxml.c

mv $PWD/ezxml_fuzz $EVAL_DIR/ezxml_fuzz
