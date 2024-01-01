#/bin/bash

EVAL_DIR=`pwd`

# download binutils
wget https://ftp.gnu.org/gnu/binutils/binutils-2.32.tar.gz
tar -xzf binutils-2.32.tar.gz
cd binutils-2.32
#make distclean
#find . -type f -name "config.cache" -delete

# build normal version for tracing
CFLAGS="-g -O0" ./configure --disable-shared
make
#make VERBOSE=1 1> make.log 2>&1

mv $PWD/binutils/readelf $EVAL_DIR/readelf_trace
