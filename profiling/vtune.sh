#!/bin/bash -e

rm -rf vtune.out

/opt/intel/oneapi/vtune/latest/bin64/vtune -collect hotspots  \
  -no-summary -result-dir vtune.out --  \
    ../build_release/asteria fib_test.ast 32
