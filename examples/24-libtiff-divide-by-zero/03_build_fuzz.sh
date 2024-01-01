#/bin/bash

EVAL_DIR=`pwd`
RACING_DIR=/home/user/docker_share/tool/Racing/release/Racing-final


if [ -f "$EVAL_DIR/temp_data/inst_id" ]; then
    rm "$EVAL_DIR/temp_data/inst_id"
    rm "$EVAL_DIR/temp_data/bb_id" 
    rm "$EVAL_DIR/temp_data/trace-id.log" 
fi

cd tiff-4.0.7
make clean

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$RACING_DIR/afl-clang-fast LD=$CC CFLAGS="-g -O0 -poc_trace=$EVAL_DIR/temp_data/poc.addr2line" ./configure --disable-shared
make

mv $PWD/tools/tiffcp $EVAL_DIR/tiffcp_fuzz