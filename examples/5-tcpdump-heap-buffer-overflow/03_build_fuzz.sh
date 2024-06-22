#/bin/bash

EVAL_DIR=`pwd`
RACING_DIR=${RACING_DIR:=/Racing-eval}


if [ -f "$EVAL_DIR/temp_data/inst_id" ]; then
    rm "$EVAL_DIR/temp_data/inst_id"
    rm "$EVAL_DIR/temp_data/bb_id" 
    rm "$EVAL_DIR/temp_data/trace-id.log" 
fi

cd tcpdump-4.9.2
make clean

export AFL_CC=clang-6.0

# build instrument version for fuzzing
CC=$RACING_DIR/Racing-code/afl-clang-fast CFLAGS="-g -O0 -fsanitize=address -poc_trace=$EVAL_DIR/temp_data/poc.addr2line" ./configure --disable-shared
make

mv $PWD/tcpdump $EVAL_DIR/tcpdump_fuzz
