#/bin/bash

EVAL_DIR=`pwd`

# git clone binutils
git clone https://github.com/bminor/binutils-gdb.git
cd binutils-gdb
git checkout 561bf3e


# build normal version for tracing
CFLAGS="-g -O0 -Wno-error" ./configure --disable-shared --disable-gdb --disable-libdecnumber --disable-readline --disable-sim --disable-ld --enable-targets=all
make

mv $PWD/binutils/objdump $EVAL_DIR/objdump_trace
