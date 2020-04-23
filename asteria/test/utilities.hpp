// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_UTILITIES_HPP_
#define ASTERIA_TEST_UTILITIES_HPP_

#include "../src/fwd.hpp"
#include "../src/utilities.hpp"

#define ASTERIA_TEST_CHECK(expr)  \
    do  \
      try {  \
        if(expr) {  \
          /* successful */  \
          break;  \
        }  \
        /* failed */  \
        ASTERIA_TERMINATE("ASTERIA_TEST_CHECK: $1\n  failed", #expr);  \
      }  \
      catch(exception& stdex) {  \
        /* failed */  \
        ASTERIA_TERMINATE("ASTERIA_TEST_CHECK: $1\n  caught an exception: $2", #expr, stdex.what());  \
      }  \
      /* unreachable */  \
    while(false)

#define ASTERIA_TEST_CHECK_CATCH(expr)  \
    do  \
      try {  \
        static_cast<void>(expr);  \
        /* failed */  \
        ASTERIA_TERMINATE("ASTERIA_TEST_CHECK_CATCH: $1\n  didn't catch an exception", #expr);  \
      }  \
      catch(exception& /*stdex*/) {  \
        /* successful */  \
        break;  \
      }  \
      /* unreachable */  \
    while(false)

#endif
