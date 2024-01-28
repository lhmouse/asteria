#!/bin/bash -e

_pkgname=asteria
_pkgversion=$(printf "0.%u.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)")
_pkgarch=$(dpkg --print-architecture)
_tempdir=$(readlink -f "./.makedeb")

rm -rf ${_tempdir}
mkdir -p ${_tempdir}
cp -pr DEBIAN -t ${_tempdir}

meson setup -Dbuildtype=release build_makedeb
meson compile -Cbuild_makedeb
DESTDIR=${_tempdir} meson install --strip -Cbuild_makedeb

sed -i "s/{_pkgname}/${_pkgname}/" ${_tempdir}/DEBIAN/control
sed -i "s/{_pkgversion}/${_pkgversion}/" ${_tempdir}/DEBIAN/control
sed -i "s/{_pkgarch}/${_pkgarch}/" ${_tempdir}/DEBIAN/control

dpkg-deb --root-owner-group --build .makedeb "${_pkgname}_${_pkgversion}_${_pkgarch}.deb"
