// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "utilities.hpp"
#include "../src/runtime/variable.hpp"

using namespace Asteria;

int main()
  {
    auto var = ::rocket::make_refcnt<Variable>();
    ASTERIA_TEST_CHECK(!var->is_initialized());

    var->initialize(V_real(123.456), false);
    ASTERIA_TEST_CHECK(var->is_initialized());
    ASTERIA_TEST_CHECK(var->get_value().is_real());

    var->open_value() = V_string(::rocket::sref("hello"));
    ASTERIA_TEST_CHECK(var->is_initialized());
    ASTERIA_TEST_CHECK(var->get_value().is_string());

    var->uninitialize();
    ASTERIA_TEST_CHECK(!var->is_initialized());
  }
