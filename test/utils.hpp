// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_UTILS_
#define ASTERIA_TEST_UTILS_

#include "../src/fwd.hpp"
#include "../src/utils.hpp"
#include <unistd.h>   // ::alarm()

#define ASTERIA_TEST_CHECK(expr)  \
    do  \
      try {  \
        if(static_cast<bool>(expr) == false) {  \
          /* failed */  \
          ::asteria::write_log_to_stderr(__FILE__, __LINE__, __func__,  \
              ::rocket::sref("ASTERIA_TEST_CHECK FAIL: " #expr));  \
          \
          ::abort();  \
        }  \
        \
        /* successful */  \
        ::asteria::write_log_to_stderr(__FILE__, __LINE__, __func__,  \
            ::rocket::sref("ASTERIA_TEST_CHECK PASS: " #expr));  \
      }  \
      catch(exception& stdex) {  \
        /* failed */  \
        ::asteria::write_log_to_stderr(__FILE__, __LINE__, __func__,  \
            ::rocket::cow_string("ASTERIA_TEST_CHECK EXCEPTION: " #expr)  \
              + "\n" + stdex.what());  \
        \
        ::abort();  \
      }  \
    while(false)

#define ASTERIA_TEST_CHECK_CATCH(expr)  \
    do  \
      try {  \
        (void) (expr);  \
        \
        /* failed */  \
        ::asteria::write_log_to_stderr(__FILE__, __LINE__, __func__,  \
            ::rocket::sref("ASTERIA_TEST_CHECK XPASS: " #expr));  \
        \
        ::abort();  \
      }  \
      catch(exception& stdex) {  \
        /* successful */  \
        ::asteria::write_log_to_stderr(__FILE__, __LINE__, __func__,  \
            ::rocket::cow_string("ASTERIA_TEST_CHECK XFAIL: " #expr)  \
              + "\n" + stdex.what());  \
      }  \
    while(false)

// Set terminate handler.
static const auto asteria_test_terminate = ::std::set_terminate(
    [] {
      auto eptr = ::std::current_exception();
      if(eptr) {
        try {
          ::std::rethrow_exception(eptr);
        }
        catch(::std::exception& stdex) {
          ::fprintf(stderr,
              "`::std::terminate()` called after `::std::exception`:\n%s\n",
              stdex.what());
        }
        catch(...) {
          ::fprintf(stderr,
              "`::std::terminate()` called after an unknown exception\n");
        }
      }
      else
        ::fprintf(stderr,
            "`::std::terminate()` called without an exception\n");

      ::fflush(nullptr);
      ::_Exit(1);
    });

// Set kill timer.
static const auto asteria_test_alarm = ::alarm(30);

#endif
