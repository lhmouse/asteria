#!/bin/bash -e

# setup
export CXX=${CXX:-"g++"}
export CXXFLAGS='-O0 -g0'

# build
${CXX} --version
mkdir -p m4
autoreconf -ifv

cd $(mktemp -d)
trap 'rm -rf ~+ || true' EXIT
~-/configure --build=${CONFIGURE_BUILD} --host=${CONFIGURE_HOST}  \
    ${CONFIGURE_OPTS} --disable-dependency-tracking --disable-silent-rules  \
    --enable-debug-checks

# test
if ! make -j$(nproc) distcheck DISTCHECK_CONFIGURE_FLAGS=${CONFIGURE_OPTS}
then
  find . -name 'test-suite.log' -exec cat '{}' ';'
  exit 1
fi
