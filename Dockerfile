FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y wget curl make gcc g++ clang-6.0 autoconf pkg-config git python3 parallel flex bison libtool zlib1g-dev ruby cmake nasm texinfo libjpeg-dev

RUN mkdir /Racing-eval && cd /Racing-eval \
    && wget -q -c http://software.intel.com/sites/landingpage/pintool/downloads/pin-3.15-98253-gb56e429b1-gcc-linux.tar.gz \
    && tar -xzf pin*.tar.gz

WORKDIR /Racing-eval
# RUN git clone https://github.com/0xdd96/Racing-final
ADD InstTracer InstTracer
ADD Racing-code Racing-code
ADD scripts scripts

ENV PIN_ROOT="/Racing-eval/pin-3.15-98253-gb56e429b1-gcc-linux"
ENV RACING_DIR="/Racing-eval"

ENV LLVM_CONFIG=llvm-config-6.0
ENV CC=clang-6.0
ENV CXX=clang++-6.0
ENV AFL_CC=clang-6.0
ENV AFL_CXX=clang++-6.0

RUN cd ${RACING_DIR}/InstTracer && CC=gcc CXX=g++ make
RUN cd ${RACING_DIR}/Racing-code && make
RUN cd ${RACING_DIR}/Racing-code/llvm_mode && make

# set up aurora
RUN git clone https://github.com/RUB-SysSec/aurora

ENV AURORA_GIT_DIR=/Racing-eval/aurora
RUN cd ${AURORA_GIT_DIR}/tracing && CC=gcc CXX=g++ make \
    && mkdir -p ${PIN_ROOT}/source/tools/AuroraTracer \
    && cp -r ${AURORA_GIT_DIR}/tracing/* ${PIN_ROOT}/source/tools/AuroraTracer

RUN wget -q -c https://lcamtuf.coredump.cx/afl/releases/afl-latest.tgz && tar xf afl-latest.tgz && mv afl-2.52b afl-fuzz
##  apply patch & build
RUN cd afl-fuzz && patch -p1 < ${AURORA_GIT_DIR}/crash_exploration/crash_exploration.patch \
    && make -j \
    && cd llvm_mode && CC=gcc CXX=g++ make ../afl-clang-fast ../afl-llvm-pass.so ../afl-llvm-rt.o && make test_build

# Install Rust
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | bash -s -- -q -y --default-toolchain nightly
ENV PATH="${HOME}/.cargo/bin:${PATH}"