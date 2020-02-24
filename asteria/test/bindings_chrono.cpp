// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(::rocket::sref(
      R"__(
        assert std.chrono.utc_format(std.numeric.integer_min) == "1601-01-01 00:00:00";
        assert std.chrono.utc_format(std.numeric.integer_min, true) == "1601-01-01 00:00:00.000";
        assert std.chrono.utc_format(std.numeric.integer_max) == "9999-01-01 00:00:00";
        assert std.chrono.utc_format(std.numeric.integer_max, true) == "9999-01-01 00:00:00.000";

        var s = "1934-05-06 07:08:09.234";
        var t = std.chrono.utc_parse(s);
        assert t;
        assert std.chrono.utc_format(t, true) == s;

        s = "2132-02-29 23:40:50.987";
        t = std.chrono.utc_parse(s);
        assert t;
        assert std.chrono.utc_format(t, true) == s;

        s = "1000-01-01 01:02:03";
        t = std.chrono.utc_parse(s);
        assert t == std.numeric.integer_min;

        s = "9999-01-01 01:02:03";
        t = std.chrono.utc_parse(s);
        assert t == std.numeric.integer_max;

        s = "invalid";
        try { t = std.chrono.utc_parse(s);  assert false;  }
          catch(e) { assert std.string.find(e, "assertion failure") == null;  }
      )__"), tinybuf::open_read);

    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    code.execute(global);
  }
