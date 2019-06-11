#!/bin/bash -e

# setup
test -n "${CXX}"  \
  || CXX="g++"

# build
${CXX} --version
./check_includes.sh
mkdir -p m4
autoreconf -ifv
./configure --disable-silent-rules ${_options}
make -j$(nproc)

# test
make -j$(nproc) check  \
  || (cat ./test-suite.log; false)
