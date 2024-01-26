#!/bin/bash -e

meson setup -Dbuildtype=debug -Denable-debug-checks=true build_ci
cd build_ci
ninja -v test
