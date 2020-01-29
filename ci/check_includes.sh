#!/bin/bash -e

test -n "${CXX}" || CXX=g++
CPPFLAGS="-D_FILE_OFFSET_BITS=64 -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE -D_WIN32_WINNT=0x0600"
CXXFLAGS="-std=gnu++14"

export CXX
export CPPFLAGS
export CXXFLAGS

_sem="parallel --will-cite --semaphore"

for _file in $(find -L "asteria" -name "*.[hc]pp")
do
  _cmd="${CXX} ${CPPFLAGS} ${CXXFLAGS} -x c++ -fsyntax-only -I."
  echo "Checking \`#include\` directives:  ${_cmd}  \"${_file}\""
  ${_sem} -j+0 -- ${_cmd}  "${_file}"
done
${_sem} --wait
