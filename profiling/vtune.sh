#!/bin/bash -e

rm -rf vtune.out

../libtool --mode=execute --  \
  /opt/intel/oneapi/vtune/latest/bin64/vtune -collect hotspots  \
    -no-summary -result-dir vtune.out --  \
    ../bin/asteria fib_test.ast 31
