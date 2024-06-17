#/bin/bash

EVAL_DIR=`pwd`

# git clone perl
# git clone https://github.com/Perl/perl5.git
# cd perl5
# git checkout dca9f61
wget https://github.com/Perl/perl5/archive/dca9f615c2ca4c784ef9cdd9a7a313de40998bcf.tar.gz
tar xzf dca9f615c2ca4c784ef9cdd9a7a313de40998bcf.tar.gz
mv perl5-dca9f615c2ca4c784ef9cdd9a7a313de40998bcf perl5
cd perl5

# build normal version for tracing
mkdir build
./Configure -Dprefix=$PWD/build -A ccflags="-g -O0 -fPIC" -A ldflags="-g -O0" -Dusedevel -d -e -s
make -j4 install.perl
mv $PWD/build/bin/perl5.31.7 $EVAL_DIR/perl_trace

