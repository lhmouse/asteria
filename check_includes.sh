#!/bin/bash

set -e

function check_one(){
	_file="$1"
	_cmd="gcc -std=c++11 -x c++ -c -o /dev/null"
	echo "Checking \`#include\` directives:  ${_cmd}  \"${_file}\""
	${_cmd}  "${_file}"
}

export -f check_one
find -L "asteria/src/" -name "*.hpp" -print0 | xargs -0 -I {} bash -c 'check_one "$@"' "$0" {}
