#/bin/bash

EVAL_DIR=`pwd`
RACING_DIR=${RACING_DIR:=/Racing-eval}


if [ -f "$EVAL_DIR/temp_data/inst_id" ]; then
    rm "$EVAL_DIR/temp_data/inst_id"
    rm "$EVAL_DIR/temp_data/bb_id" 
    rm "$EVAL_DIR/temp_data/trace-id.log" 
fi

cd screen-4.7.0
make clean

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CFLAGS="-g -O0" ./configure --disable-shared
CC=$RACING_DIR/Racing-code/afl-clang-fast CFLAGS="-g -O0 -poc_trace=$EVAL_DIR/temp_data/poc.addr2line" ./configure --enable-rxvt_osc --disable-shared
make

mv $PWD/screen $EVAL_DIR/screen_fuzz
