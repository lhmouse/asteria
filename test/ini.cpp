// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      &__FILE__, __LINE__, &R"__(
///////////////////////////////////////////////////////////////////////////////

        std.debug.logf("__file = $1", __file);
        const path = std.string.pcre_replace(__file, '/ini\.cpp$', '/test.ini');
        const obj = std.ini.parse_file(path);

        assert obj.global == "value";
        assert obj.section1.key == "value";
        assert obj.section1."some crazy" == "spaces";
        assert obj.section2."key without a value" == "";

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
