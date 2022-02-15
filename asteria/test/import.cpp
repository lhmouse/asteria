// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"

using namespace ::asteria;

int main()
  {
    const ::rocket::unique_ptr<char, void (&)(void*)> abspath(::realpath(__FILE__, nullptr), ::free);
    ROCKET_ASSERT(abspath);

    Simple_Script code;
    code.reload_string(
      cow_string(abspath), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        std.debug.logf("__file = $1", __file);
        assert import("import_sub.txt", 3, 5) == -2;
        assert import("import_sub.txt", 3, 5,) == -2;

        assert catch( import("nonexistent file") ) != null;

        try {
          import("import_recursive.txt");
          assert false;
        }
        catch(e)
          assert std.string.find(e, "recursive import") != null;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
