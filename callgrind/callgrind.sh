#!/bin/bash -e

../libtool --mode=execute valgrind --tool=callgrind --dump-instr=yes  \
    --callgrind-out-file=callgrind.out --  \
    ../bin/asteria fib27.ast
