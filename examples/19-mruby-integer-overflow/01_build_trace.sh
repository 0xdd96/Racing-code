#/bin/bash

EVAL_DIR=`pwd`

# clone mruby
git clone https://github.com/mruby/mruby.git
cd mruby
git checkout e9ddb59

# build normal version for tracing
CFLAGS="-g -O0 -fPIC" ./minirake $PWD/bin/mruby

mv $PWD/bin/mruby $EVAL_DIR/mruby_trace
