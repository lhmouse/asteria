#!/bin/bash -e

# setup
export CXX=${CXX:-"g++"}
export CFLAGS='-O0 -g0'
export DISTCHECK_CONFIGURE_FLAGS="${CONFIGURE_OPTS}"

# build
if test "${CONFIGURE_BUILD}" != ""
then
  DISTCHECK_CONFIGURE_FLAGS="--build=${CONFIGURE_BUILD} ${DISTCHECK_CONFIGURE_FLAGS}"
fi

if test "${CONFIGURE_HOST}" != ""
then
  DISTCHECK_CONFIGURE_FLAGS="--host=${CONFIGURE_HOST} ${DISTCHECK_CONFIGURE_FLAGS}"
  CXX="${CONFIGURE_HOST}-g++"
fi

${CXX} --version
mkdir -p m4
autoreconf -ifv

cd $(mktemp -d)
trap 'rm -rf ~+ || true' EXIT
~-/configure ${DISTCHECK_CONFIGURE_FLAGS}  \
    --disable-dependency-tracking --disable-silent-rules --enable-debug-checks

# test
if ! make -j$(nproc) distcheck
then
  find . -name 'test-suite.log' -exec cat '{}' ';'
  exit 1
fi
