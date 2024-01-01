#/bin/bash

EVAL_DIR=`pwd`
RACING_DIR=/home/user/docker_share/tool/Racing/release/Racing-final


if [ -f "$EVAL_DIR/temp_data/inst_id" ]; then
    rm "$EVAL_DIR/temp_data/inst_id"
    rm "$EVAL_DIR/temp_data/bb_id" 
    rm "$EVAL_DIR/temp_data/trace-id.log" 
fi

cd binutils-gdb
make distclean
find . -type f -name "config.cache" -delete
if grep -q "case \" \${CFLAGS} \" in
  *\\ -fsanitize=address\\ *) NOASANFLAG=-fno-sanitize=address ;;$" ./libiberty/configure; then
  sed -i '5189,5191d' ./libiberty/configure
fi

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$RACING_DIR/afl-clang-fast LD=$CC CFLAGS="-g -O0 -fsanitize=address -Wno-error -poc_trace=$EVAL_DIR/temp_data/poc.addr2line" ./configure --disable-shared --disable-gdb --disable-libdecnumber --disable-readline --disable-sim --disable-ld --enable-targets=all
make

mv $PWD/binutils/objdump $EVAL_DIR/objdump_fuzz
