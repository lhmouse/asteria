// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
#include "../asteria/runtime/garbage_collector.hpp"
#include "../asteria/runtime/variable.hpp"
#include "../rocket/xmemory.hpp"
using namespace ::asteria;

atomic_relaxed<long> bcnt;

void* operator new(size_t cb)
  {
    auto ptr = ::std::malloc(cb);
    if(!ptr)
      throw ::std::bad_alloc();

    bcnt.xadd(1);
    return ptr;
  }

void operator delete(void* ptr) noexcept
  {
    if(!ptr)
      return;

    bcnt.xsub(1);
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

    bcnt.store(0);
    {
      Simple_Script code;
      code.reload_string(
        sref(__FILE__), __LINE__, sref(""
        "const nloop = 10000;"
        R"__(
///////////////////////////////////////////////////////////////////////////////

          var g;
          func leak() {
            var f;
            f = func() { return f; };
            var k = f;
            g = k;
          }
          for(var i = 0;  i < nloop;  ++i) {
            leak();
          }

///////////////////////////////////////////////////////////////////////////////
        )__"));
      code.execute();
    }

    ::rocket::xmemclean();
    ::rocket::xmemclean();
    ASTERIA_TEST_CHECK(bcnt.load() == 0);
  }
