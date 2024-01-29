#!/bin/bash -e

_pkgname=asteria
_pkgversion=$(git describe --tag | sed 's/^[^0-9]*//')
_pkgarch=$(dpkg --print-architecture)
_tempdir=$(readlink -f "./.makedeb")

rm -rf ${_tempdir}
mkdir -p ${_tempdir}
cp -pr DEBIAN -t ${_tempdir}

export CFLAGS='-O2 -g'
export CXXFLAGS='-O2 -g'

meson setup -Dbuildtype=plain build_makedeb
meson compile -Cbuild_makedeb
DESTDIR=${_tempdir} meson install --strip -Cbuild_makedeb

sed -i "s/{_pkgname}/${_pkgname}/" ${_tempdir}/DEBIAN/control
sed -i "s/{_pkgversion}/${_pkgversion}/" ${_tempdir}/DEBIAN/control
sed -i "s/{_pkgarch}/${_pkgarch}/" ${_tempdir}/DEBIAN/control

dpkg-deb --root-owner-group --build .makedeb "${_pkgname}_${_pkgversion}_${_pkgarch}.deb"
