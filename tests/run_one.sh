#!/bin/bash

if [[ "$#" -lt 1 ]]; then
	printf '\e[31;1mFAILED   No source file provided.\e[0m\n' >&2
	exit 2;
fi

_file="$(readlink -e "$1")"
if [[ -z "${_file}" ]]; then
	printf '\e[31;1mFAILED   Source file '"'$1'"' not found.\e[0m\n' >&2
	exit 3;
fi
shift

_cxx="g++"
_cxxflags="-std=c++11 -O0 -g3 -fPIC -Wall -Wextra -Werror -pedantic -pedantic-errors"
_ldflags="-L../lib -L../lib/.libs"
_runpath="../lib/.libs"
_libs="-lasteria"
_output="test.out~"
_errlog="stderr.log~"

_cmd=$(cat <<EOF
	set -e
	"${_cxx}" ${CPPFLAGS} ${_cxxflags} ${CXXFLAGS} ${_ldflags} ${LDFLAGS} "${_file}" ${_libs} -o "${_output}" $*
	LD_LIBRARY_PATH="${_runpath}" "./${_output}"
EOF
)

printf '\e[33;1mRUNNING  %s\e[0m\n' "${_file}"
if ! sh -c "${_cmd}" >&2; then
	printf '\e[31;1mFAILED   %s\n%s\e[0m\n' "${_file}" "${_cmd}"
	exit 1
fi
printf '\e[32;1mPASSED   %s\e[0m\n' "${_file}"
