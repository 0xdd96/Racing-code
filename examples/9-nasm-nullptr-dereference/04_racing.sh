#!/bin/bash
EVAL_DIR=`pwd`
AFL_DIR=/home/user/docker_share/tool/Racing/release/Racing-final

i=0

while [ $i -lt 5 ]
do
        AFL_WORKDIR=$EVAL_DIR/afl-workdir-batch$i
        mkdir -p $AFL_WORKDIR
        rm $EVAL_DIR/seed/*.o
        # run AFL
	timeout 43200 $AFL_DIR/afl-fuzz -C -d -m none -i $EVAL_DIR/seed -o $AFL_WORKDIR -s $EVAL_DIR/temp_data/trace-id.log -- $EVAL_DIR/nasm_fuzz -f elf -o /dev/null @@
        i=$(($i+1));
done

