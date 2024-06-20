#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../binutils-gdb
make distclean
find . -type f -name "config.cache" -delete
if grep -q "case \" \${CFLAGS} \" in
  *\\ -fsanitize=address\\ *) NOASANFLAG=-fno-sanitize=address ;;$" ./libiberty/configure; then
  sed -i '5189,5191d' ./libiberty/configure
fi

export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$AFL_DIR/afl-clang-fast LD=$CC CFLAGS="-g -O0 -fsanitize=address -Wno-error" ./configure --disable-shared --disable-gdb --disable-libdecnumber --disable-readline --disable-sim --disable-ld --enable-targets=all
make

mv $PWD/binutils/objdump $EVAL_DIR/objdump_fuzz.aurora