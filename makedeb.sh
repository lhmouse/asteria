#!/bin/bash -e

_pkgname="asteria"
_pkggroup="asteria"
_pkgsource="https://github.com/lhmouse/asteria"
_pkglicense="BSD 3-Clause License"

_pkgversion=$(git describe 2>/dev/null ||
              printf "0.%u.%s" "$(git rev-list --count HEAD)"  \
                               "$(git rev-parse --short HEAD)")

_maintainer=$(printf "%s <%s>" "$(git config --get user.name)"  \
                               "$(git config --get user.email)")

sudo ./ci/checkinstall  \
  --backup=no --nodoc --default --strip=no --stripso=no --addso=yes  \
  --pkgname="${_pkgname}"  \
  --pkggroup="${_pkggroup}"  \
  --pkgsource="${_pkgsource}"  \
  --pkglicense="${_pkglicense}"  \
  --pkgversion="${_pkgversion}"  \
  --pkgrelease=1  \
  --maintainer="${_maintainer}"  \
  --exclude="${_tmpdir}"

sudo ldconfig -v
