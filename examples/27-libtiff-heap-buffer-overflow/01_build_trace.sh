#/bin/bash

EVAL_DIR=`pwd`

# download libtiff
wget https://download.osgeo.org/libtiff/tiff-4.0.6.tar.gz
tar -xzf tiff-4.0.6.tar.gz
cd tiff-4.0.6

# build normal version for tracing
CFLAGS="-g -O0" ./configure --disable-shared
make

mv $PWD/tools/tiffcrop $EVAL_DIR/tiffcrop_trace


