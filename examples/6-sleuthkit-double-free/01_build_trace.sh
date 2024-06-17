#/bin/bash

EVAL_DIR=`pwd`

# clone sleuthkit
# git clone https://github.com/sleuthkit/sleuthkit.git
# cd sleuthkit
# git checkout 3e2332a
wget https://github.com/sleuthkit/sleuthkit/archive/3e2332a1f1917dd62ad41f84e296439ff87b878f.tar.gz
tar xzf 3e2332a1f1917dd62ad41f84e296439ff87b878f.tar.gz
mv sleuthkit-3e2332a1f1917dd62ad41f84e296439ff87b878f sleuthkit
cd sleuthkit

# build normal version for tracing
./bootstrap
CFLAGS="-g -O0 -fPIC" CXXFLAGS=$CFLAGS ./configure --disable-shared
make

mv $PWD/tools/fstools/fls $EVAL_DIR/fls_trace
