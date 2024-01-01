#/bin/bash

EVAL_DIR=`pwd`

# git clone perl
git clone https://github.com/Perl/perl5.git
cd perl5
git checkout dca9f61

# build normal version for tracing
mkdir build
./Configure -Dprefix=$PWD/build -A ccflags="-g -O0 -fPIC" -A ldflags="-g -O0"
make install.perl
mv $PWD/build/bin/perl $EVAL_DIR/perl_trace

