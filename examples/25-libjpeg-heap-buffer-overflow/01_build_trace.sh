#/bin/bash

EVAL_DIR=`pwd`

# clone libjpeg
git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git
cd libjpeg-turbo
git checkout f4b8a5c

# build normal version for tracing
mkdir build
cd build
CFLAGS="-g -O0" CXXFLAGS=$CFLAGS cmake -G "Unix Makefiles" -DBUILD_SHARED_LIBS=off ..
make

mv $PWD/cjpeg $EVAL_DIR/cjpeg_trace
