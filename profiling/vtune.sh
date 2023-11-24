#!/bin/bash -e

../libtool --mode=execute -- \
  /opt/intel/oneapi/vtune/latest/bin64/vtune -collect hotspots -- \
    ../bin/asteria fib_test.ast
