#!/bin/bash -e

# setup
test -n "${CXX}" || CXX="g++"
export CXX

# build
${CXX} --version
mkdir -p m4
autoreconf -ifv
./configure --disable-silent-rules --enable-debug-checks
make -j$(nproc)

# test
./check_includes.sh
make -j$(nproc) check || (cat ./test-suite.log; false)
