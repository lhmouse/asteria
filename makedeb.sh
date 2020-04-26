#!/bin/bash -e

_pkgname="asteria"
_maintainer="$(git config --get user.email)"
_pkgversion="$(git describe 2>/dev/null ||
               printf "0.%u.%s" "$(git rev-list --count HEAD)"  \
                                "$(git rev-parse --short HEAD)")"

sudo checkinstall --backup=no --nodoc -y --strip=no --stripso=no  \
  --addso=yes --pkgname="${_pkgname}" --maintainer="${_maintainer}"  \
  --pkgversion="${_pkgversion}"
