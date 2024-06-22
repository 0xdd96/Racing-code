#/bin/bash

EVAL_DIR=`pwd`
export PIN_ROOT="${PIN_ROOT:=/Racing-eval/pin-3.15-98253-gb56e429b1-gcc-linux}"

mkdir -p $EVAL_DIR/traces
# requires at least python 3.6
cd $AURORA_GIT_DIR/tracing/scripts

python3 tracing.py $EVAL_DIR/mruby_trace $EVAL_DIR/inputs $EVAL_DIR/traces
# extract stack and heap addr ranges from logfiles
python3 addr_ranges.py --eval_dir $EVAL_DIR $EVAL_DIR/traces

