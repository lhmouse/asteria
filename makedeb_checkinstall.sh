#!/bin/bash -e

meson setup -Ddebug=true -Doptimization=3 build_makedeb
meson compile -C build_makedeb
meson test -C build_makedeb

sudo checkinstall  \
  --pkgname="asteria-local"  \
  --pkgversion="$(git describe --tags | sed 's/^[^0-9]*//')"  \
  --pkgsource="https://github.com/lhmouse/asteria"  \
  --pkglicense="BSD-3-Clause"  \
  --pkggroup="devel"  \
  --pkgarch="$(dpkg --print-architecture)"  \
  --nodoc --backup=no --default --fstrans=no --install=yes  \
  --  \
  meson install -C build_makedeb
