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

        var output;

        output = "";
        for(each k, v -> ["a","b","c"])
          output += std.string.format("$1=$2;", k, v);
        assert output == "0=a;1=b;2=c;";

        output = "";
        for(each k: v -> ["a","b","c"])
          output += std.string.format("$1=$2;", k, v);
        assert output == "0=a;1=b;2=c;";

        output = "";
        for(each k = v -> ["a","b","c"])
          output += std.string.format("$1=$2;", k, v);
        assert output == "0=a;1=b;2=c;";

        output = "";
        for(each v -> ["a","b","c"])
          output += std.string.format("$1;", v);
        assert output == "a;b;c;";

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
