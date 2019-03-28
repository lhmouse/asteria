// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/runtime/variable.hpp"

using namespace Asteria;

int main()
  {
    auto var = rocket::make_refcnt<Variable>(Source_Location(rocket::sref("test"), 42));
    ASTERIA_TEST_CHECK(var->get_value().type() == dtype_null);

    var->reset(Source_Location(rocket::sref("test"), 43), D_real(123.456), false);
    ASTERIA_TEST_CHECK(var->get_value().type() == dtype_real);

    var->open_value() = D_string(rocket::sref("hello"));
    ASTERIA_TEST_CHECK(var->get_value().type() == dtype_string);
  }
