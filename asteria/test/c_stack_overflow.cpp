// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"
#include <pthread.h>

using namespace asteria;

static void* thread_proc(void*)
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        var a;
        for(var k = 0; k < 100000; ++k)
          a = [ a ];

        std.io.putln("meow");
        std.io.flush();

        std.system.gc_collect();
        a = null;
        std.system.gc_collect();

        std.io.putln("bark");
        std.io.flush();

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
    return nullptr;
  }

int main()
  {
    ::pthread_attr_t attr;
    ASTERIA_TEST_CHECK(::pthread_attr_init(&attr) == 0);
    ASTERIA_TEST_CHECK(::pthread_attr_setguardsize(&attr, 0x2000) == 0);
    ASTERIA_TEST_CHECK(::pthread_attr_setstacksize(&attr, 0xE000) == 0);

    ::pthread_t thrd;
    ASTERIA_TEST_CHECK(::pthread_create(&thrd, &attr, thread_proc, nullptr) == 0);
    ASTERIA_TEST_CHECK(::pthread_join(thrd, nullptr) == 0);
  }
