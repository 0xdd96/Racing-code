#/bin/bash

EVAL_DIR=`pwd`

# clone patch
git clone https://git.savannah.gnu.org/git/patch.git
cd patch
git checkout dce4683

# build normal version for tracing
./bootstrap
CFLAGS="-g -O0 -fPIC"  ./configure --disable-shared
make

mv $PWD/src/patch $EVAL_DIR/patch_trace
