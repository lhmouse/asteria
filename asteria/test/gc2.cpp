// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/garbage_collector.hpp"
#include "../src/runtime/variable.hpp"
#include <algorithm>

using namespace ::asteria;

sso_vector<void*, 1000> alloc_list;
sso_vector<void*, 1000> free_list;

void* operator new(size_t cb)
  {
    auto ptr = ::std::malloc(cb);
    if(!ptr)
      throw ::std::bad_alloc();

    alloc_list.push_back(ptr);
    return ptr;
  }

void operator delete(void* ptr) noexcept
  {
    if(!ptr)
      return;

    free_list.push_back(ptr);
    ::std::free(ptr);
  }

void operator delete(void* ptr, size_t) noexcept
  {
    ::operator delete(ptr);
  }

int main()
  {
    // Ignore leaks of emutls, emergency pool, etc.
    delete new int;

    {
      Simple_Script code;
      code.reload_string(
        sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

          (func(){
            (func(){
              var x;
              func f() { return x;  }
              func g() { return f;  }
              x = g;
            }());
          }());

          assert std.system.gc_collect() == 3;  // x, f, g

///////////////////////////////////////////////////////////////////////////////
        )__"));
      code.execute();
    }

    std::sort(alloc_list.mut_begin(), alloc_list.mut_end());
    std::sort(free_list.mut_begin(), free_list.mut_end());
    ASTERIA_TEST_CHECK(::std::equal(
           alloc_list.begin(), alloc_list.end(), free_list.begin(), free_list.end()));
  }
