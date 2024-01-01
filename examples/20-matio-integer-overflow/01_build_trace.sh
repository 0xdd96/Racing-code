#/bin/bash

EVAL_DIR=`pwd`

# clone matio
git clone https://github.com/tbeu/matio.git
cd matio
git checkout bcf0447

# build normal version for tracing
./autogen.sh
CFLAGS="-g -O0" ./configure --disable-shared
make

mv $PWD/tools/matdump $EVAL_DIR/matdump_trace
