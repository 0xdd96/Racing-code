#/bin/bash

EVAL_DIR=`pwd`

# clone libxml2
git clone https://github.com/GNOME/libxml2.git
cd libxml2
git checkout 362b322

# build normal version for tracing
./autogen.sh
CFLAGS="-g -O0" ./configure --disable-shared
make

mv $PWD/xmllint $EVAL_DIR/xmllint_trace
