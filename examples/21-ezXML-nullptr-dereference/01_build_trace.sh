#/bin/bash

EVAL_DIR=`pwd`

# download libpng
wget http://prdownloads.sourceforge.net/ezxml/ezxml-0.8.6.tar.gz
tar -xzf ezxml-0.8.6.tar.gz
cd ezxml

# build normal version for tracing
gcc -Wall -O0 -DEZXML_TEST -g -ggdb -o ezxml_trace ezxml.c

mv $PWD/ezxml_trace $EVAL_DIR/ezxml_trace


