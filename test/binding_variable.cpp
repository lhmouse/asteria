// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
#include "../asteria/runtime/variable.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;

    auto var = code.open_global_variable(&"some_fancy_variable");
    var->initialize(42);  // integer
    ASTERIA_TEST_CHECK(var->get_value().is_integer());
    ASTERIA_TEST_CHECK(var->get_value().as_integer() == 42);

    code.reload_string(
      &__FILE__, __LINE__, &R"__(
///////////////////////////////////////////////////////////////////////////////

        // read the old value
        assert some_fancy_variable == 42;
        assert extern some_fancy_variable == 42;

        // change it!
        some_fancy_variable = "meow!";

        // read the new value
        assert extern some_fancy_variable == "meow!";

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();

    ASTERIA_TEST_CHECK(var->get_value().is_string());
    ASTERIA_TEST_CHECK(var->get_value().as_string() == "meow!");
  }
