// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/compiler/simple_source_file.hpp"
#include "../src/runtime/global_context.hpp"
#include <sstream>

using namespace Asteria;

int main()
  {
    static constexpr char s_source[] =
      R"__(
        assert std.chrono.datetime_min() == "1601-01-01 00:00:00";
        assert std.chrono.datetime_min(true) == "1601-01-01 00:00:00.000";

        assert std.chrono.datetime_max() == "9999-01-01 00:00:00";
        assert std.chrono.datetime_max(true) == "9999-01-01 00:00:00.000";

        assert std.chrono.datetime_format(std.numeric.integer_min) == std.chrono.datetime_min();
        assert std.chrono.datetime_format(std.numeric.integer_max) == std.chrono.datetime_max();

        var s, t;

        s = "1934-05-06 07:08:09.234";
        t = std.chrono.datetime_parse(s);
        assert t;
        assert std.chrono.datetime_format(t, true) == s;

        s = "2132-02-29 23:40:50.987";
        t = std.chrono.datetime_parse(s);
        assert t;
        assert std.chrono.datetime_format(t, true) == s;

        s = "1000-01-01 01:02:03";
        t = std.chrono.datetime_parse(s);
        assert t == std.numeric.integer_min;

        s = "9999-01-01 01:02:03";
        t = std.chrono.datetime_parse(s);
        assert t == std.numeric.integer_max;

        s = "invalid";
        t = std.chrono.datetime_parse(s);
        assert t == null;
      )__";

    std::istringstream iss(s_source);
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global, { });
  }
