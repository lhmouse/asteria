#!/bin/bash -e

meson setup -Dbuildtype=debug build_debug
meson setup -Dbuildtype=release build_release
