#/bin/bash

EVAL_DIR=`pwd`
RACING_DIR=/home/user/docker_share/tool/Racing/release/Racing-final


if [ -f "$EVAL_DIR/temp_data/inst_id" ]; then
    rm "$EVAL_DIR/temp_data/inst_id"
    rm "$EVAL_DIR/temp_data/bb_id" 
    rm "$EVAL_DIR/temp_data/trace-id.log" 
fi

cd libjpeg-turbo-2.0.90
rm build -r
mkdir build
cd build

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$RACING_DIR/afl-clang-fast CXX=$RACING_DIR/afl-clang-fast++ CFLAGS="-g -O0 -fsanitize=address -poc_trace=$EVAL_DIR/temp_data/poc.addr2line" CXXFLAGS=$CFLAGS cmake -G "Unix Makefiles" -DBUILD_SHARED_LIBS=off ..
make

mv $PWD/cjpeg $EVAL_DIR/cjpeg_fuzz
