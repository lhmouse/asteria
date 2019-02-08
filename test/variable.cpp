// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/runtime/variable.hpp"

using namespace Asteria;

int main()
  {
    const auto var = rocket::make_refcnt<Variable>();
    ASTERIA_TEST_CHECK(var->get_value().type() == type_null);

    var->reset(D_real(123.456), false);
    ASTERIA_TEST_CHECK(var->get_value().type() == type_real);

    var->open_value() = D_string(rocket::sref("hello"));
    ASTERIA_TEST_CHECK(var->get_value().type() == type_string);

    var->reset(D_integer(42), true);  // immutable
    ASTERIA_TEST_CHECK_CATCH(var->open_value());
  }
