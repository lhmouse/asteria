// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/runtime/variable.hpp"

using namespace ::asteria;

int main()
  {
    auto var = ::rocket::make_refcnt<Variable>();
    ASTERIA_TEST_CHECK(var->is_uninitialized());

    var->initialize(V_real(123.456));
    ASTERIA_TEST_CHECK(!var->is_uninitialized());
    ASTERIA_TEST_CHECK(var->get_value().is_real());

    var->open_value() = V_string(sref("hello"));
    ASTERIA_TEST_CHECK(!var->is_uninitialized());
    ASTERIA_TEST_CHECK(var->get_value().is_string());

    var->uninitialize();
    ASTERIA_TEST_CHECK(var->is_uninitialized());
  }
