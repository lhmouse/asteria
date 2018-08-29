// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/exception.hpp"

using namespace Asteria;

int main()
  {
    try {
      try {
        throw Exception(Reference_root::S_constant { D_integer(42) });
      } catch(Exception &e) {
        const auto val = e.get_reference().read();
        ASTERIA_TEST_CHECK(val.type() == Value::type_integer);
        ASTERIA_TEST_CHECK(val.check<D_integer>() == 42);
        e.set_reference(Reference(Reference_root::S_temporary { D_string(String::shallow("hello")) }));
        throw;
      }
    } catch(Exception &e) {
      const auto val = e.get_reference().read();
      ASTERIA_TEST_CHECK(val.type() == Value::type_string);
      ASTERIA_TEST_CHECK(val.check<D_string>() == "hello");
    }
  }
