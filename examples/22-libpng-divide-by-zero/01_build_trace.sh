#/bin/bash

EVAL_DIR=`pwd`

# download libpng
wget https://downloads.sourceforge.net/libpng/libpng-1.6.34.tar.gz
tar -xzf libpng-1.6.34.tar.gz
cd libpng-1.6.34

# build normal version for tracing
sed -i 's/return ((int)(crc != png_ptr->crc));/return (0);/g' pngrutil.c
./autogen.sh
CFLAGS="-g -O0" ./configure --disable-shared
make 

mv $PWD/pngimage $EVAL_DIR/pngimage_trace
