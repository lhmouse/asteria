#!/bin/bash -e

meson setup -Dbuildtype=debug -Denable-debug-checks=true build_debug
meson setup -Dbuildtype=release build_release
