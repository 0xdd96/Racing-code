#!/bin/bash
EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:=/Racing-eval/Racing-code}
RUN_N=${RUN_N:-1}


AFL_WORKDIR=$EVAL_DIR/afl-workdir-batch0
mkdir -p $AFL_WORKDIR
# run AFL
timeout 43200 $AFL_DIR/afl-fuzz -C -d -m none -i $EVAL_DIR/seed -o $AFL_WORKDIR -s $EVAL_DIR/temp_data/trace-id.log -- $EVAL_DIR/ezxml_fuzz @@