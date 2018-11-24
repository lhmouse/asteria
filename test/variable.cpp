// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/runtime/variable.hpp"
#include "../asteria/src/runtime/source_location.hpp"

using namespace Asteria;

int main()
  {
    const auto var = rocket::make_refcounted<Variable>(Source_location(rocket::cow_string::shallow("nonexistent"), 42), D_real(123.456), false);
    ASTERIA_TEST_CHECK(var->get_value().type() == Value::type_real);
    var->set_value(D_string(rocket::cow_string::shallow("hello")));
    ASTERIA_TEST_CHECK(var->get_value().type() == Value::type_string);
    var->reset(D_integer(42), true);
    ASTERIA_TEST_CHECK_CATCH(var->set_value(D_null()));
  }
