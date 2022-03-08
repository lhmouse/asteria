// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"

using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        var namespace = {

        var value = 1;
        var xnul;

        foo1 = 1; bar1 = 2;  // plain fields

        func get_value()
          { return this.value;  }

        func set_value(x)
          { this.value = x;  }

        foo2 = 1; bar2 = 2;  // plain fields
        "no trailing comma" = true

        };

        assert namespace.get_value() == 1;

        namespace.value = "meow";
        assert namespace.get_value() == "meow";

        namespace.set_value(namespace.xnul);
        assert namespace.get_value() == null;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
