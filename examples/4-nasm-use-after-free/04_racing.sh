#!/bin/bash
EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:=/Racing-eval/Racing-code}
RUN_N=${RUN_N:-1}

i=0

while [ $i -lt $RUN_N ]
do
        AFL_WORKDIR=$EVAL_DIR/afl-workdir-batch$i
        mkdir -p $AFL_WORKDIR
        rm $EVAL_DIR/seed/*.o
        # run AFL
	$AFL_DIR/afl-fuzz -C -d -m none -i $EVAL_DIR/seed -o $AFL_WORKDIR -s $EVAL_DIR/temp_data/trace-id.log -- $EVAL_DIR/nasm_fuzz -o outfile @@
        i=$(($i+1));
done

