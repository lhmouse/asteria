// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_UTILITIES_HPP_
#define ASTERIA_TEST_UTILITIES_HPP_

#include "../src/fwd.hpp"
#include "../src/runtime/traceable_exception.hpp"
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
      catch(::Asteria::Traceable_Exception& e) {  \
        /* success */  \
        ASTERIA_DEBUG_LOG("Caught `Asteria::Traceable_Exception`: ", e.value());  \
        for(::std::size_t i = 0; i < e.frame_count(); ++i) {  \
          ASTERIA_DEBUG_LOG("\t* thrown from `", e.frame(i).function_signature(), "` at '", e.frame(i).source_location(), "']");  \
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
