// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TEST_UTILS_
#define ASTERIA_TEST_UTILS_

#include "../asteria/fwd.hpp"
#include "../asteria/utils.hpp"

#define ASTERIA_TEST_CHECK(expr)  \
    do  \
      try {  \
        if(static_cast<bool>(expr) == false) {  \
          /* failed */  \
          ::rocket::cow_string msg;  \
          msg << "ASTERIA_TEST_CHECK FAIL: " #expr;  \
          ASTERIA_WRITE_STDERR(move(msg));  \
          ::abort();  \
        }  \
        \
        /* successful */  \
        ::rocket::cow_string msg;  \
        msg << "ASTERIA_TEST_CHECK PASS: " #expr;  \
        ASTERIA_WRITE_STDERR(move(msg));  \
      }  \
      catch(::std::exception& stdex) {  \
        /* failed */  \
        ::rocket::cow_string msg;  \
        msg << "ASTERIA_TEST_CHECK EXCEPT: " #expr;  \
        msg << "\n" << stdex.what();  \
        ASTERIA_WRITE_STDERR(move(msg));  \
        ::abort();  \
      }  \
    while(false)

#define ASTERIA_TEST_CHECK_CATCH(expr)  \
    do  \
      try {  \
        (void) (expr);  \
        \
        /* failed */  \
        ::rocket::cow_string msg;  \
        msg << "ASTERIA_TEST_CHECK XPASS: " #expr;  \
        ASTERIA_WRITE_STDERR(move(msg));  \
        ::abort();  \
      }  \
      catch(::std::exception& stdex) {  \
        /* successful */  \
        ::rocket::cow_string msg;  \
        msg << "ASTERIA_TEST_CHECK XFAIL: " #expr;  \
        msg << "\n" << stdex.what();  \
        ASTERIA_WRITE_STDERR(move(msg));  \
      }  \
    while(false)

// Set terminate handler.
static const auto asteria_test_terminate = ::std::set_terminate(
  [] {
    try {
      if(auto eptr = ::std::current_exception())
        ::std::rethrow_exception(eptr);
      else
        ::fprintf(stderr, "terminated without an exception\n");
    }
    catch(::std::exception& stdex)
    { ::fprintf(stderr, "terminated after `::std::exception`:\n%s\n", stdex.what());  }
    catch(...)
    { ::fprintf(stderr, "terminated after an unknown exception\n");  }

    ::fflush(nullptr);
    ::_Exit(1);
  });

#endif
