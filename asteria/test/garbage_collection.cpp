// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

std::atomic<long> bcnt;

void* operator new(size_t cb)
  {
    auto ptr = std::malloc(cb);
    if(!ptr) {
      throw std::bad_alloc();
    }
    bcnt.fetch_add(1, std::memory_order_relaxed);
    return ptr;
  }

void operator delete(void* ptr) noexcept
  {
    if(!ptr) {
      return;
    }
    bcnt.fetch_sub(1, std::memory_order_relaxed);
    std::free(ptr);
  }

void operator delete(void* ptr, size_t) noexcept
  {
    operator delete(ptr);
  }

int main()
  {
    // Ignore leaks of emutls, emergency pool, etc.
    rocket::make_unique<int>().reset();

    bcnt.store(0, std::memory_order_relaxed);
    {
      rocket::cow_stringbuf buf;
      buf.set_string(rocket::sref(
        R"__(
          var g;
          func leak() {
            var f;
            f = func() { return f; };
            g = f;
          }
          for(var i = 0; i < 1000000; ++i) {
            leak();
          }
        )__"), std::ios_base::in);
      Simple_Script code(buf, rocket::sref("my_file"));
      Global_Context global;
      code.execute(global);
    }
    ASTERIA_TEST_CHECK(bcnt.load(std::memory_order_relaxed) == 0);
  }
