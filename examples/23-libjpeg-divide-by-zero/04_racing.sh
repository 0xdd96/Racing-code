#!/bin/bash
EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:=/Racing-eval/Racing-code}
RUN_N=${RUN_N:-1}

i=0

while [ $i -lt $RUN_N ]
do
        AFL_WORKDIR=$EVAL_DIR/afl-workdir-batch$i
        mkdir -p $AFL_WORKDIR
        # run AFL
	$AFL_DIR/afl-fuzz -C -d -m none -i $EVAL_DIR/seed -o $AFL_WORKDIR -s $EVAL_DIR/temp_data/trace-id.log -- $EVAL_DIR/cjpeg_fuzz @@
        i=$(($i+1));
done

