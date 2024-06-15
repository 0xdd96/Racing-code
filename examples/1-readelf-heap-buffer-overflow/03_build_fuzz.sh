#/bin/bash

EVAL_DIR=`pwd`
RACING_DIR=${RACING_DIR:=/Racing-eval}

if [ -f "$EVAL_DIR/temp_data/inst_id" ]; then
    rm "$EVAL_DIR/temp_data/inst_id"
    rm "$EVAL_DIR/temp_data/bb_id" 
    rm "$EVAL_DIR/temp_data/trace-id.log" 
fi

cd binutils-2.32
make distclean
find . -type f -name "config.cache" -delete
if grep -q "case \" \${CFLAGS} \" in
  *\\ -fsanitize=address\\ *) NOASANFLAG=-fno-sanitize=address ;;$" ./libiberty/configure; then
  sed -i '5262,5264d' ./libiberty/configure
fi


export AFL_CC=clang-6.0
# build instrument version for fuzzing
CC=$RACING_DIR/Racing-code/afl-clang-fast  CFLAGS="-g -O0 -fsanitize=address -poc_trace=$EVAL_DIR/temp_data/poc.addr2line" ./configure --disable-shared
ASAN_OPTIONS=detect_leaks=0 make

mv $PWD/binutils/readelf $EVAL_DIR/readelf_fuzz
