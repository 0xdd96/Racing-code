#/bin/bash

EVAL_DIR=`pwd`

# download libsixel
wget https://github.com/libsixel/libsixel/archive/refs/tags/v1.8.4.tar.gz
tar -xzf v1.8.4.tar.gz
cd libsixel-1.8.4

# build normal version for tracing
CFLAGS="-g -O0" ./configure --disable-shared
make

mv $PWD/converters/img2sixel $EVAL_DIR/img2sixel_trace
