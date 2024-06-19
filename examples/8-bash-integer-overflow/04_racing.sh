#!/bin/bash
EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:=/Racing-eval/Racing-code}
RUN_N=${RUN_N:-5}

i=0

mkdir -p run_temp

while [ $i -lt $RUN_N ]
do
        AFL_WORKDIR=$EVAL_DIR/afl-workdir-batch$i
        mkdir -p $AFL_WORKDIR
        # run AFL
	(cd run_temp; timeout 43200 $AFL_DIR/afl-fuzz -C -d -m none -i $EVAL_DIR/seed -o $AFL_WORKDIR -s $EVAL_DIR/temp_data/trace-id.log -- $EVAL_DIR/bash_fuzz)
        i=$(($i+1));
done

