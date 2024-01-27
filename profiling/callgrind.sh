#!/bin/bash -e

rm -rf callgrind.out

valgrind --tool=callgrind --callgrind-out-file=callgrind.out  \
  --dump-instr=yes --  \
    ../build_release/asteria fib_test.ast 27
