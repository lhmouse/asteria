// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_UTIL_HPP_
#define ASTERIA_TEST_UTIL_HPP_

#include "../src/fwd.hpp"
#include "../src/util.hpp"
#include <unistd.h>   // ::alarm()

#define ASTERIA_TEST_CHECK(expr)  \
    do  \
      try {  \
        if(static_cast<bool>(expr) == false) {  \
          /* failed */  \
          ::asteria::write_log_to_stderr(__FILE__, __LINE__,  \
              ::asteria::format_string(  \
                 "ASTERIA_TEST_CHECK FAIL: $1",  \
                 #expr  \
              ));  \
          ::abort();  \
          break;  \
        }  \
        \
        /* successful */  \
        ::asteria::write_log_to_stderr(__FILE__, __LINE__,  \
            ::asteria::format_string(  \
               "ASTERIA_TEST_CHECK PASS: $1",  \
               #expr  \
            ));  \
      }  \
      catch(exception& stdex) {  \
        /* failed */  \
        ::asteria::write_log_to_stderr(__FILE__, __LINE__,  \
            ::asteria::format_string(  \
               "ASTERIA_TEST_CHECK EXCEPTION: $1\n$2",  \
               #expr, stdex  \
            ));  \
        ::abort();  \
      }  \
    while(false)

#define ASTERIA_TEST_CHECK_CATCH(expr)  \
    do  \
      try {  \
        static_cast<void>(expr);  \
        \
        /* failed */  \
        ::asteria::write_log_to_stderr(__FILE__, __LINE__,  \
            ::asteria::format_string(  \
               "ASTERIA_TEST_CHECK_CATCH XPASS: $1",  \
               #expr  \
            ));  \
        ::abort();  \
      }  \
      catch(exception& stdex) {  \
        /* successful */  \
        ::asteria::write_log_to_stderr(__FILE__, __LINE__,  \
            ::asteria::format_string(  \
               "ASTERIA_TEST_CHECK XFAIL: $1",  \
               #expr  \
            ));  \
      }  \
    while(false)

// Set kill timer.
static const auto asteria_test_alarm = ::alarm(30);

#endif
