#/bin/bash

EVAL_DIR=`pwd`
export PIN_ROOT=/home/user/evaluation/pin-3.15-98253-gb56e429b1-gcc-linux
TOOL_DIR=/home/user/docker_share/tool/Racing/release

mkdir $EVAL_DIR/temp_data

cd $TOOL_DIR/scripts/
python3 tracing.py "$EVAL_DIR/tcpdump_trace -vr @@" $EVAL_DIR/seed/tcpdump-heapoverread $EVAL_DIR/temp_data/poc.addr

# Maps binary addresses to source code locations using addr2line.
python3 addr2line.py $EVAL_DIR/tcpdump_trace $EVAL_DIR/temp_data/poc.addr $EVAL_DIR/temp_data/poc.addr2line

