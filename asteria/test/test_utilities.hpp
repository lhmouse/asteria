// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_UTILITIES_HPP_
#define ASTERIA_TEST_UTILITIES_HPP_

#include "../src/fwd.hpp"
#include "../src/runtime/runtime_error.hpp"
#include "../src/utilities.hpp"

#define ASTERIA_TEST_CHECK(expr_)  \
    do {  \
      try {  \
        if(expr_) {  \
          /* success */  \
          break;  \
        }  \
        /* failure */  \
        ::std::fprintf(stderr, "ASTERIA_TEST_CHECK() failed: %s\n  at %s:%ld\n",  \
                               #expr_, __FILE__, (long)__LINE__);  \
        ::std::terminate();  \
        /* unreachable */  \
      }  \
      catch(::std::exception& e_) {  \
        /* failure */  \
        ::std::fprintf(stderr, "ASTERIA_TEST_CHECK() caught an exception: %s\n  what = %s\n  at %s:%ld\n",  \
                               #expr_, e_.what(), __FILE__, (long)__LINE__);  \
        ::std::terminate();  \
      }  \
      /* unreachable */  \
    } while(false)

#define ASTERIA_TEST_CHECK_CATCH(expr_)  \
    do {  \
      try {  \
        static_cast<void>(expr_);  \
        /* failure */  \
      }  \
      catch(::Asteria::Runtime_Error& e_) {  \
        /* success */  \
        break;  \
      }  \
      catch(::std::exception& e_) {  \
        /* success */  \
        break;  \
      }  \
      ::std::fprintf(stderr, "ASTERIA_TEST_CHECK_CATCH() didn't catch an exception: %s\n  at %s:%ld\n",  \
                             #expr_, __FILE__, (long)__LINE__);  \
      ::std::terminate();  \
      /* unreachable */  \
    } while(false)

#endif
