// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_UTILITIES_HPP_
#define ASTERIA_TEST_UTILITIES_HPP_

#include "../src/fwd.hpp"
#include "../src/runtime/exception.hpp"
#include "../src/utilities.hpp"
#include <iostream>  // std::cerr, operator<< ()

#define ASTERIA_TEST_CHECK(expr_)  \
    do {  \
      if(expr_) {  \
        /* success */  \
        break;  \
      }  \
      /* failure */  \
      ::std::cerr << "ASTERIA_TEST_CHECK() failed: " << #expr_ << '\n'  \
                  << "  File: " << __FILE__ << '\n'  \
                  << "  Line: " << __LINE__ << '\n'  \
                  << ::std::flush;  \
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
        for(::size_t i = e.count_frames() - 1; i != SIZE_MAX; --i) {  \
          const auto& f = e.frame(i);  \
          ASTERIA_DEBUG_LOG("\t* thrown from \'", f.sloc(), "\' <", f.what_type(), ">: ", f.value());  \
        }  \
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
      ::std::cerr << "ASTERIA_TEST_CHECK_CATCH() didn't catch an exception: " << #expr_ << '\n'  \
                  << "  File: " << __FILE__ << '\n'  \
                  << "  Line: " << __LINE__ << '\n'  \
                  << ::std::flush;  \
      ::std::terminate();  \
      /* unreachable */  \
    } while(false)

#endif
