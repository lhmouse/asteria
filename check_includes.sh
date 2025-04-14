#!/bin/bash -e

# setup
export CXXFLAGS="-D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ibuild_debug -std=c++17 -mavx"

if test -z ${CXX}
then
  CXX=g++
fi

for _f in $(find ~+/"asteria" -name "*.[hc]pp")
do
  echo "Checking includes:  ${_f}"
  ${CXX} ${CXXFLAGS} -x c++ ${_f} -S -o /dev/null
done
