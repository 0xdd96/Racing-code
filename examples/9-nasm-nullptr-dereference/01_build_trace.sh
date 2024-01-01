#/bin/bash

EVAL_DIR=`pwd`

# download nasm
wget https://github.com/netwide-assembler/nasm/archive/refs/tags/nasm-2.14rc15.tar.gz
tar -xzf nasm-2.14rc15.tar.gz
cd nasm-nasm-2.14rc15

# build normal version for tracing
./autogen.sh
CFLAGS="-g -O0 -fPIC" ./configure --disable-shared
make

mv $PWD/nasm $EVAL_DIR/nasm_trace
