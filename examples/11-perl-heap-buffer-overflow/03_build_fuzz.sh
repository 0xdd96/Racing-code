#/bin/bash

EVAL_DIR=`pwd`
RACING_DIR=${RACING_DIR:=/Racing-eval}


if [ -f "$EVAL_DIR/temp_data/inst_id" ]; then
    rm "$EVAL_DIR/temp_data/inst_id"
    rm "$EVAL_DIR/temp_data/bb_id" 
    rm "$EVAL_DIR/temp_data/trace-id.log" 
fi

export AFL_CC=clang-6.0
# build instrument version for fuzzing
cd perl5
make clean
./Configure -Dprefix=$PWD/build -DCC=$RACING_DIR/Racing-code/afl-clang-fast -A ccflags="-g -O0 -fsanitize=address -fPIC -poc_trace=$EVAL_DIR/temp_data/poc.addr2line" -Dld=$RACING_DIR/afl-clang-fast -A ldflags="-g -O0 -fsanitize=address"
make install.perl
mv $PWD/build/bin/perl $EVAL_DIR/perl_fuzz
