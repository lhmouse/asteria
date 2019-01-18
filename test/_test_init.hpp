// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_INIT_HPP_
#define ASTERIA_TEST_INIT_HPP_

#ifndef ENABLE_DEBUG_LOGS
#  define ENABLE_DEBUG_LOGS   1
#endif

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
        ::std::cout << "ASTERIA_TEST_CHECK_CATCH() caught `Asteria::Traceable_Exception`:\n"  \
                    << "\t" << e.get_value() << ::std::endl;  \
        auto &bt = e.get_backtrace();  \
        for(auto it = bt.rbegin(); it != bt.rend(); ++it) {  \
          ::std::cout << "\t* thrown from: " << *it << ::std::endl;  \
        }  \
        break;  \
      } catch(::Asteria::Runtime_Error &e) {  \
        ::std::cout << "ASTERIA_TEST_CHECK_CATCH() caught `Asteria::Runtime_Error`:\n\t"  \
                    << e.what() << ::std::endl;  \
        break;  \
      } catch(::std::exception &e) {  \
        ::std::cout << "ASTERIA_TEST_CHECK_CATCH() caught `std::exception`:\n\t"  \
                    << e.what() << ::std::endl;  \
        break;  \
      }  \
      ::std::cerr << "ASTERIA_TEST_CHECK_CATCH() didn't catch an exception: " << #expr_ << '\n'  \
                  << "  File: " << __FILE__ << '\n'  \
                  << "  Line: " << __LINE__ << '\n'  \
                  << ::std::flush;  \
      ::std::terminate();  \
    } while(false)

#endif
