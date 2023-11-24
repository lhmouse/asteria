#!/bin/bash -e

../libtool --mode=execute -- \
  valgrind --tool=callgrind --dump-instr=yes  \
    --fn-skip=mem{set,cpy,move} --callgrind-out-file=callgrind.out --  \
    ../bin/asteria fib_test.ast
