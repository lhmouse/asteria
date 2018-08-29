// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/exception.hpp"

using namespace Asteria;

int main()
  {
    try {
      try {
        Reference ref(Reference_root::S_constant { D_integer(42) });
        throw Exception(std::move(ref));
      } catch(Exception &e) {
        const auto val = read_reference(e.get_reference());
        ASTERIA_TEST_CHECK(val.type() == Value::type_integer);
        ASTERIA_TEST_CHECK(val.check<D_integer>() == 42);
        e.set_reference(Reference(Reference_root::S_temporary { D_string(String::shallow("hello")) }));
        throw;
      }
    } catch(Exception &e) {
      const auto val = read_reference(e.get_reference());
      ASTERIA_TEST_CHECK(val.type() == Value::type_string);
      ASTERIA_TEST_CHECK(val.check<D_string>() == "hello");
    }
  }
