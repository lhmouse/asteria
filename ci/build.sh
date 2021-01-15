#!/bin/bash -e

# setup
export CXX=${CXX:-"g++"}
export CXXFLAGS='-O2 -g0'

_nocheck=

while test $# -gt 0
do
  case $1 in
    --disable-make-check )
      _nocheck=1
      shift 1
      ;;

    * )
      echo "WARNING: unknown option -- '$1'" >&2
      shift 1
      ;;
  esac
done

# build
${CXX} --version
mkdir -p m4
autoreconf -ifv
./configure --disable-silent-rules --enable-debug-checks --disable-static
make -j$(nproc)

# test
if ! test -n "$_nocheck"
then
  make -j$(nproc) check || (cat ./test-suite.log; false)
fi
