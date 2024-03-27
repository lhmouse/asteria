#!/bin/bash -e

# setup
export CXX=${CXX:-"g++"}
export CXXFLAGS="-D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809
                 -D_GNU_SOURCE -Ibuild_debug -std=c++17"

# note: `sem` is not always available
_sem="parallel --will-cite --semaphore --halt soon,fail=1"

for _f in $(find "rocket" "asteria" -name "*.[hc]pp")
do
  echo "Checking includes:  ${_f}"
  ${_sem} -j+0 -- ${CXX} ${CXXFLAGS} -x c++ ${_f} -S -o /dev/null
done
${_sem} --wait
