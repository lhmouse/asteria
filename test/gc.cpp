// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/garbage_collector.hpp"
#include "../src/runtime/variable.hpp"
#include <algorithm>

using namespace ::asteria;

sso_vector<void*, 10000> alloc_list;
sso_vector<void*, 10000> free_list;

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

    auto foreign = ::rocket::make_refcnt<Variable>();
    foreign->initialize(42);
    {
      Simple_Script code;
      code.global().open_named_reference(sref("foreign_variable")).set_variable(foreign);

      code.reload_string(
        sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

          ref gr -> __global foreign_variable;

          (func() {
            (func() {
              var x,y,z;

              func foo() { return [x,y,z,gr];  }
              func bar() { return [z,y,x,gr];  }

              x = [foo,bar,foo,bar,foo];
              y = [x,[bar,foo,bar]];
              z = x;
              y = x;
              gr = y;
            }());
          }());

          assert std.system.gc_collect() == 2;  // foo,bar
          gr = "meow";
          assert std.system.gc_collect() == 3;  // x,y,z

///////////////////////////////////////////////////////////////////////////////
        )__"));
      code.execute();
    }
    ASTERIA_TEST_CHECK(foreign->use_count() == 1);
    ASTERIA_TEST_CHECK(foreign->is_uninitialized() == false);
    ASTERIA_TEST_CHECK(foreign->get_value().type() == type_string);
    foreign = nullptr;

    std::sort(alloc_list.mut_begin(), alloc_list.mut_end());
    std::sort(free_list.mut_begin(), free_list.mut_end());
    ASTERIA_TEST_CHECK(::std::equal(
           alloc_list.begin(), alloc_list.end(), free_list.begin(), free_list.end()));
  }
