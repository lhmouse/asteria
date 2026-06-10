#!/bin/bash -e

_version=$(git describe --tags | sed 's/^[^0-9]*//')
_arch=$(dpkg --print-architecture)

meson setup  \
  -Ddebug=true -Doptimization=3  \
  --prefix="/usr/local"  \
  "build_makedeb/build"

meson compile -C "build_makedeb/build"
meson test -C "build_makedeb/build"

rm -rf "build_makedeb/pkg"
mkdir -p "build_makedeb/pkg/DEBIAN"

DESTDIR=$(realpath "build_makedeb/pkg") meson install -C "build_makedeb/build"

tee "build_makedeb/pkg/DEBIAN/control" <<control_EOF
Package: asteria-local
Provides: asteria, libasteria-dev
Version: ${_version}
Architecture: ${_arch}
Multi-Arch: same
Maintainer: LH_Mouse <lh_mouse@126.com>
Homepage: https://github.com/lhmouse/asteria
License: BSD-3-Clause
Description: The Asteria Programming Language
 Asteria (as-TEAR-ee-uh) is a procedural, dynamically typed programming
 language that is highly inspired by JavaScript.
control_EOF

tee "build_makedeb/pkg/DEBIAN/postinst" <<postinst_EOF
#!/bin/sh -e
ldconfig
postinst_EOF

chmod +x "build_makedeb/pkg/DEBIAN/postinst"
cp -p "build_makedeb/pkg/DEBIAN/"post{inst,rm}

dpkg-deb --root-owner-group --build "build_makedeb/pkg"  \
  "asteria-local_${_version}_${_arch}.deb"
