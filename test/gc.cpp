// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
#include "../asteria/runtime/garbage_collector.hpp"
#include "../asteria/runtime/variable.hpp"
#include "../rocket/xmemory.hpp"
#include <algorithm>
using namespace ::asteria;

static_vector<void*, 10000> alloc_list;
static_vector<void*, 10000> free_list;

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
      code.mut_global().insert_named_reference(&"foreign_variable").set_variable(foreign);

      code.reload_string(
        &__FILE__, __LINE__, &R"__(
///////////////////////////////////////////////////////////////////////////////

          ref gr -> extern foreign_variable;

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

          assert std.gc.collect() == 2;  // foo,bar
          gr = "meow";
          assert std.gc.collect() == 3;  // x,y,z

///////////////////////////////////////////////////////////////////////////////
        )__");
      code.execute();
    }

    ASTERIA_TEST_CHECK(foreign->use_count() == 1);
    ASTERIA_TEST_CHECK(foreign->initialized() == true);
    ASTERIA_TEST_CHECK(foreign->value().type() == type_string);
    foreign = nullptr;

    ::rocket::xmemclean();
    ::std::sort(alloc_list.mut_begin(), alloc_list.mut_end());
    ::std::sort(free_list.mut_begin(), free_list.mut_end());
    ASTERIA_TEST_CHECK(::std::equal(
           alloc_list.begin(), alloc_list.end(), free_list.begin(), free_list.end()));
  }
