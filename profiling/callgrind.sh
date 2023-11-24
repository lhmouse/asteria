#!/bin/bash -e

rm -rf callgrind.out

../libtool --mode=execute --  \
  valgrind --tool=callgrind --dump-instr=yes  \
    --callgrind-out-file=callgrind.out --   \
    ../bin/asteria fib_test.ast 27
