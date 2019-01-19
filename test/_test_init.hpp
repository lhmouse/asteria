// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_INIT_HPP_
#define ASTERIA_TEST_INIT_HPP_

#include "../asteria/src/fwd.hpp"
#include "../asteria/src/runtime/traceable_exception.hpp"
#include "../asteria/src/utilities.hpp"
#include <iostream>  // std::cerr, operator<< ()

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
      } catch(::Asteria::Traceable_Exception &e) {  \
        ASTERIA_DEBUG_LOG("Caught `Asteria::Traceable_Exception`: ", e.get_value());  \
        for(const auto &loc : e.get_backtrace()) {  \
          ASTERIA_DEBUG_LOG("\t* thrown from: ", loc);  \
        }  \
        break;  \
      } catch(::Asteria::Runtime_Error &e) {  \
        ASTERIA_DEBUG_LOG("Caught `Asteria::Runtime_Error`: ", e.what());  \
        break;  \
      } catch(::std::exception &e) {  \
        ASTERIA_DEBUG_LOG("Caught `std::exception`: ", e.what());  \
        break;  \
      }  \
      ::std::cerr << "ASTERIA_TEST_CHECK_CATCH() didn't catch an exception: " << #expr_ << '\n'  \
                  << "  File: " << __FILE__ << '\n'  \
                  << "  Line: " << __LINE__ << '\n'  \
                  << ::std::flush;  \
      ::std::terminate();  \
    } while(false)

#endif
