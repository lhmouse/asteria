#!/bin/bash -e

test -n "${CXX}" || CXX=g++
export CXX

function check_one()
  {
    _cmd="${CXX} -std=c++11 -x c++ -c -o /dev/null"
    echo "Checking \`#include\` directives:  ${_cmd}  \"$1\""
    ${_cmd}  "$1"
  }
export -f check_one

find -L "asteria/src/" -name "*.hpp" -print0 |  \
  xargs -0 -P$(nproc) -I{} bash -ec 'check_one "$@"' "$0" {}
