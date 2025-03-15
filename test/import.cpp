// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      get_real_path(&__FILE__), __LINE__, &R"__(
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
          assert std.string.find(e, "Recursive import") != null;

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
