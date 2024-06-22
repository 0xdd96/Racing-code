#/bin/bash

EVAL_DIR=`pwd`

# clone bash
# git clone https://github.com/bminor/bash.git
# cd bash
# git checkout 6444760
wget https://github.com/bminor/bash/archive/64447609994bfddeef1061948022c074093e9a9f.tar.gz
tar xzf 64447609994bfddeef1061948022c074093e9a9f.tar.gz
mv bash-64447609994bfddeef1061948022c074093e9a9f bash
cd bash

# build normal version for tracing
mkdir build
CFLAGS="-g -O0 -fPIC"  ./configure --prefix=$PWD/build --disable-shared
make
make install

mv $PWD/build/bin/bash $EVAL_DIR/bash_trace
