#!/bin/bash -e

# setup
export CXX=${CXX:-"g++"}
export CXXFLAGS

# build
${CXX} --version
mkdir -p m4
autoreconf -ifv
./configure --disable-silent-rules --enable-debug-checks
make -j$(nproc)

# test
./check_includes.sh
make -j$(nproc) distcheck || (cat ./test-suite.log; false)
