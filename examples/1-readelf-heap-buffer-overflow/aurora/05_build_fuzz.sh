#/bin/bash

EVAL_DIR=`pwd`
AFL_DIR=${AFL_DIR:-/Racing-eval/afl-fuzz}
AURORA_GIT_DIR=${AURORA_GIT_DIR:-/Racing-eval/aurora}

cd ../binutils-2.32
make distclean
find . -type f -name "config.cache" -delete
if grep -q "case \" \${CFLAGS} \" in
  *\\ -fsanitize=address\\ *) NOASANFLAG=-fno-sanitize=address ;;$" ./libiberty/configure; then
  sed -i '5262,5264d' ./libiberty/configure
fi


export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$AFL_DIR/afl-clang-fast  CFLAGS="-g -O0 -fsanitize=address" ./configure --disable-shared
ASAN_OPTIONS=detect_leaks=0 make

mv $PWD/binutils/readelf $EVAL_DIR/readelf_fuzz.aurora
