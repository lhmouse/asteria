#!/bin/bash -e

meson setup  \
  -Dbuildtype=debug  \
  -Denable-debug-checks=true  \
  -Db_sanitize=address,undefined  \
  build_debug

meson setup  \
  -Dbuildtype=release  \
  build_release
