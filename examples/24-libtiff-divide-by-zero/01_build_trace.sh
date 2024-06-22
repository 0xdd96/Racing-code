#/bin/bash

EVAL_DIR=`pwd`

# download libtiff
wget https://download.osgeo.org/libtiff/tiff-4.0.7.tar.gz
tar -xzf tiff-4.0.7.tar.gz
cd tiff-4.0.7

# build normal version for tracing
CFLAGS="-g -O0" ./configure --disable-shared
make

mv $PWD/tools/tiffcp $EVAL_DIR/tiffcp_trace


