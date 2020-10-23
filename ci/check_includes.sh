#!/bin/bash -e

# setup
export CXX=${CXX:-"g++"}
export CPPFLAGS="-D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE -D_WIN32_WINNT=0x0600"
export CXXFLAGS='-std=gnu++14 -fno-gnu-keywords -Wno-zero-as-null-pointer-constant'

# note: `sem` is not always available
_sem="parallel --will-cite --semaphore --halt soon,fail=1"

for _file in $(find -L "asteria" -name "*.[hc]pp")
do
  _cmd="${CXX} ${CPPFLAGS} ${CXXFLAGS} -x c++ -fsyntax-only -DHAVE_CONFIG_H -I."
  echo "Checking \`#include\` directives:  ${_cmd}  \"${_file}\""
  ${_sem} -j+0 -- ${_cmd}  "${_file}"
done
${_sem} --wait
