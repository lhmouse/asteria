// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "util.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    const ::rocket::unique_ptr<char, void (&)(void*)> abspath(::realpath(__FILE__, nullptr), ::free);
    ROCKET_ASSERT(abspath);

    Simple_Script code;
    code.reload_string(
      cow_string(abspath), __LINE__, ::rocket::sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        std.debug.logf("__file = $1", __file);
        assert import("import_add.txt", 3, 5) == 8;

        try { import("nonexistent file");  assert false;  }
          catch(e) { assert std.string.find(e, "Assertion failure") == null;  }

        try { import("import_recursive.txt");  assert false;  }
          catch(e) { assert std.string.find(e, "Recursive import") != null;  }

///////////////////////////////////////////////////////////////////////////////
      )__"));
    Global_Context global;
    code.execute(global);
  }
