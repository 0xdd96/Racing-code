#/bin/bash

EVAL_DIR=`pwd`

# download lua
wget https://www.lua.org/ftp/lua-5.0.tar.gz
tar -xzf lua-5.0.tar.gz
cd lua-5.0

# build normal version for tracing
./configure

MYCFLAGS="-g -O0" make -e

mv $PWD/bin/lua $EVAL_DIR/lua_trace
