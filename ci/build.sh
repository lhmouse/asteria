#!/bin/bash -e

# setup
export CXX=${CXX:-"g++"}
export CXXFLAGS='-O2 -g0'

# build
${CXX} --version
mkdir -p m4
autoreconf -ifv

cd $(mktemp -d)
trap 'rm -rf ~+ || true' EXIT
~-/configure --disable-silent-rules --enable-debug-checks --{build,host}=${CONFIGURE_HOST} ${CONFIGURE_OPTS}

# test
if ! make -j$(nproc) distcheck
then
  cat ./test-suite.log
  exit 1
fi
