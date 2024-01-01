#/bin/bash

EVAL_DIR=`pwd`

# clone sleuthkit
git clone https://github.com/sleuthkit/sleuthkit.git
cd sleuthkit
git checkout 3e2332a

# build normal version for tracing
./bootstrap
CFLAGS="-g -O0 -fPIC" CXXFLAGS=$CFLAGS ./configure --disable-shared
make

mv $PWD/tools/fstools/fls $EVAL_DIR/fls_trace
