#!/bin/bash -e

_pkgname=asteria
_pkgversion=$(printf "0.%u.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)")
_pkgarch=$(dpkg --print-architecture)

_tempdir=$(readlink -f "./.makedeb")
_debiandir=${_tempdir}/DEBIAN

rm -rf ${_tempdir}
mkdir -p ${_tempdir}
cp -pr DEBIAN -t ${_tempdir}

make install-strip DESTDIR=${_tempdir}
find ${_tempdir} -name "*.la" -delete
sed -i "s/{_pkgname}/${_pkgname}/" ${_debiandir}/control
sed -i "s/{_pkgversion}/${_pkgversion}/" ${_debiandir}/control
sed -i "s/{_pkgarch}/${_pkgarch}/" ${_debiandir}/control

dpkg-deb --root-owner-group --build .makedeb "${_pkgname}_${_pkgversion}_${_pkgarch}.deb"
