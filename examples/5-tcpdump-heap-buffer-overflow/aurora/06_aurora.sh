#!/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

AFL_WORKDIR=$EVAL_DIR/afl-workdir
mkdir -p $AFL_WORKDIR
mkdir -p $EVAL_DIR/inputs/crashes
mkdir -p $EVAL_DIR/inputs/non_crashes

# run AFL
timeout 7200 $AFL_DIR/afl-fuzz -C -d -m none -i $EVAL_DIR/../seed -o $AFL_WORKDIR -- $EVAL_DIR/tcpdump_fuzz.aurora -vr @@

# save crashes and non-crashes
cp $AFL_WORKDIR/queue/* $EVAL_DIR/inputs/crashes
cp $AFL_WORKDIR/non_crashes/* $EVAL_DIR/inputs/non_crashes
