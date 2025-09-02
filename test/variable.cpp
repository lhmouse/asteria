// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/runtime/variable.hpp"
using namespace ::asteria;

int main()
  {
    auto var = ::rocket::make_refcnt<Variable>();
    ASTERIA_TEST_CHECK(var->initialized() == false);

    var->initialize(V_real(123.456));
    ASTERIA_TEST_CHECK(var->initialized() == true);
    ASTERIA_TEST_CHECK(var->value().is_real());

    var->mut_value() = V_string(&"hello");
    ASTERIA_TEST_CHECK(var->initialized() == true);
    ASTERIA_TEST_CHECK(var->value().is_string());

    var->uninitialize();
    ASTERIA_TEST_CHECK(var->initialized() == false);
  }
