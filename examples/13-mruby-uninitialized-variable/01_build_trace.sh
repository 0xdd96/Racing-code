#/bin/bash

EVAL_DIR=`pwd`

# clone mruby
git clone https://github.com/mruby/mruby.git
cd mruby
git checkout 7483753

# build normal version for tracing
CFLAGS="-g -O0 -fPIC" ./minirake $PWD/bin/mruby
#CC=clang-6.0 CFLAGS="-g -O0 -fPIC -fsanitize=memory" LD=$CC LDFLAGS="-fsanitize=memory -fPIC -lm" ./minirake  $PWD/bin/mruby

mv $PWD/bin/mruby $EVAL_DIR/mruby_trace
