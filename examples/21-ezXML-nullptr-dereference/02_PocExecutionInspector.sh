#/bin/bash

EVAL_DIR=`pwd`
export PIN_ROOT="${PIN_ROOT:=/Racing-eval/pin-3.15-98253-gb56e429b1-gcc-linux}"
RACING_DIR=${RACING_DIR:=/Racing-eval}

mkdir -p $EVAL_DIR/temp_data

cd $RACING_DIR/scripts/
python3 tracing.py "$EVAL_DIR/ezxml_trace @@" $EVAL_DIR/seed/poc $EVAL_DIR/temp_data/poc.addr

# Maps binary addresses to source code locations using addr2line.
python3 addr2line.py $EVAL_DIR/ezxml_trace $EVAL_DIR/temp_data/poc.addr $EVAL_DIR/temp_data/poc.addr2line

