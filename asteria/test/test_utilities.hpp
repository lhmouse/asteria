// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_UTILITIES_HPP_
#define ASTERIA_TEST_UTILITIES_HPP_

#include "../src/fwd.hpp"
#include "../src/runtime/exception.hpp"
#include "../src/utilities.hpp"

#define ASTERIA_TEST_CHECK(expr_)  \
    do {  \
      if(expr_) {  \
        /* success */  \
        break;  \
      }  \
      /* failure */  \
      ::std::fprintf(stderr, "ASTERIA_TEST_CHECK() failed: %s\n  at %s:%ld\n",  \
                             #expr_, __FILE__, static_cast<long>(__LINE__));  \
      ::std::terminate();  \
      /* unreachable */  \
    } while(false)

#define ASTERIA_TEST_CHECK_CATCH(expr_)  \
    do {  \
      try {  \
        static_cast<void>(expr_);  \
        /* failure */  \
      }  \
      catch(::Asteria::Exception& e) {  \
        /* success */  \
        ASTERIA_DEBUG_LOG("Caught `Asteria::Exception`: ", e.value());  \
        break;  \
      }  \
      catch(::Asteria::Runtime_Error& e) {  \
        /* success */  \
        ASTERIA_DEBUG_LOG("Caught `Asteria::Runtime_Error`: ", e.what());  \
        break;  \
      }  \
      catch(::std::exception& e) {  \
        /* success */  \
        ASTERIA_DEBUG_LOG("Caught `std::exception`: ", e.what());  \
        break;  \
      }  \
      ::std::fprintf(stderr, "ASTERIA_TEST_CHECK_CATCH() didn't catch an exception: %s\n  at %s:%ld\n",  \
                             #expr_, __FILE__, static_cast<long>(__LINE__));  \
      ::std::terminate();  \
      /* unreachable */  \
    } while(false)

#endif
