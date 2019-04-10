// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../asteria/src/compiler/simple_source_file.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include "../asteria/src/rocket/insertable_istream.hpp"

using namespace Asteria;

std::atomic<long> bcnt;

void* operator new(std::size_t cb)
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

#if __cplusplus >= 201402
void operator delete(void* ptr, std::size_t) noexcept
  {
    operator delete(ptr);
  }
#endif

int main()
  {
    // Ignore leaks of emutls, emergency pool, etc.
    rocket::make_unique<int>().reset();

    bcnt.store(0, std::memory_order_relaxed);
    {
      rocket::insertable_istream iss(
        rocket::sref(
          R"__(
            var g;
            func leak() {
              var f;
              f = func() { return f; };
              g = f;
            }
            for(var i = 0; i < 10000; ++i) {
              leak();
            }
          )__")
        );
      Simple_Source_File code(iss, rocket::sref("my_file"));
      Global_Context global;
      code.execute(global, { });
    }
    ASTERIA_TEST_CHECK(bcnt.load(std::memory_order_relaxed) == 0);
  }
