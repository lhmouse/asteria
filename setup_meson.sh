#!/bin/bash -e

meson setup  \
  -Ddebug=true -Doptimization=0  \
  -Denable-debug-checks=true  \
  -Db_sanitize=address,undefined  \
  build_debug

meson setup  \
  -Ddebug=true -Doptimization=3  \
  build_release
