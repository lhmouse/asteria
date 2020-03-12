// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include "../src/runtime/global_context.hpp"
#include "../src/runtime/generational_collector.hpp"
#include "../src/runtime/variable.hpp"

using namespace Asteria;

::std::atomic<long> bcnt;

void* operator new(size_t cb)
  {
    auto ptr = ::std::malloc(cb);
    if(!ptr) {
      throw ::std::bad_alloc();
    }
    bcnt.fetch_add(1, ::std::memory_order_relaxed);
    return ptr;
  }

void operator delete(void* ptr) noexcept
  {
    if(!ptr) {
      return;
    }
    bcnt.fetch_sub(1, ::std::memory_order_relaxed);
    ::std::free(ptr);
  }

void operator delete(void* ptr, size_t) noexcept
  {
    operator delete(ptr);
  }

int main()
  {
    // Ignore leaks of emutls, emergency pool, etc.
    delete new int;

    rcptr<Variable> var;
    bcnt.store(0, ::std::memory_order_relaxed);
    {
      Global_Context global;
      var = global.generational_collector()->create_variable();
      var->initialize(V_string("meow"), true);

      ::rocket::tinybuf_str cbuf;
      cbuf.set_string(::rocket::sref(
#ifdef __OPTIMIZE__
        "const nloop = 1000000;"
#else
        "const nloop = 10000;"
#endif
        R"__(
          var g;
          func leak() {
            var f;
            f = func() { return f; };
            g = f;
          }
          for(var i = 0;  i < nloop;  ++i) {
            leak();
          }
        )__"), tinybuf::open_read);
      Simple_Script code(cbuf, ::rocket::sref(__FILE__));
      code.execute(global);
    }
    ASTERIA_TEST_CHECK(var->is_initialized() == false);
    var.reset();
    ASTERIA_TEST_CHECK(bcnt.load(::std::memory_order_relaxed) == 0);
  }
