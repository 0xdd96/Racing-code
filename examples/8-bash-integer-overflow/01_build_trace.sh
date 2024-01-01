#/bin/bash

EVAL_DIR=`pwd`

# clone bash
git clone https://github.com/bminor/bash.git
cd bash
git checkout 6444760

# build normal version for tracing
mkdir build
CFLAGS="-g -O0 -fPIC"  ./configure --prefix=$PWD/build --disable-shared
make
make install

mv $PWD/build/bin/bash $EVAL_DIR/bash_trace
