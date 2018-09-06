// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/variable.hpp"

using namespace Asteria;

int main()
  {
    Variable var(D_real(123.456), false);
    ASTERIA_TEST_CHECK(var.get_value().type() == Value::type_real);
    var.set_value(D_string(String::shallow("hello")));
    ASTERIA_TEST_CHECK(var.get_value().type() == Value::type_string);
    var.set_immutable(true);
    ASTERIA_TEST_CHECK_CATCH(var.set_value(D_null()));
  }
