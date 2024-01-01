#/bin/bash

EVAL_DIR=`pwd`

# download lua
wget https://ftp.gnu.org/gnu/screen/screen-4.7.0.tar.gz
tar -xzf screen-4.7.0.tar.gz
cd screen-4.7.0

# build normal version for tracing
./autogen.sh
CFLAGS="-g -O0" ./configure --enable-rxvt_osc --disable-shared
make

mv $PWD/screen $EVAL_DIR/screen_trace
