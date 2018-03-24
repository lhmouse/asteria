#!/bin/bash

set -e

_self="$(readlink -e "$0")"
_dir="$(dirname "${_self}")/tests"

cd "${_dir}"
find . -name "*.cpp" -print0 | xargs -0 -i "./run_one.sh" "{}" $*
