// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_INIT_HPP_
#define ASTERIA_TEST_INIT_HPP_

#include "../asteria/src/fwd.hpp"
#include <iostream>  // std::cerr, operator<< ()
#include <exception>  // std::terminate(), std::exception

#ifndef ENABLE_DEBUG_LOGS
#  define ENABLE_DEBUG_LOGS   1
#endif
#include "../asteria/src/utilities.hpp"

#define ASTERIA_TEST_CHECK(expr_)  \
    do {  \
      if(expr_) {  \
        break;  \
      }  \
      ::std::cerr << "ASTERIA_TEST_CHECK() failed: " << #expr_ << '\n'  \
                  << "  File: " << __FILE__ << '\n'  \
                  << "  Line: " << __LINE__ << '\n'  \
                  << ::std::flush;  \
      ::std::terminate();  \
    } while(false)

#define ASTERIA_TEST_CHECK_CATCH(expr_)  \
    do {  \
      try {  \
        static_cast<void>(expr_);  \
      } catch(::std::exception &e) {  \
        ::std::cout << "ASTERIA_TEST_CHECK_CATCH() caught: " << e.what() << ::std::endl;  \
        break;  \
      }  \
      ::std::cerr << "ASTERIA_TEST_CHECK_CATCH() didn't catch an exception: " << #expr_ << '\n'  \
                  << "  File: " << __FILE__ << '\n'  \
                  << "  Line: " << __LINE__ << '\n'  \
                  << ::std::flush;  \
      ::std::terminate();  \
    } while(false)

#endif
