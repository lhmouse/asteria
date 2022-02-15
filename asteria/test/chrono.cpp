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
        assert catch( std.chrono.utc_parse(s) ) != null;

        t = 631152000000;  // 1990-01-01 00:00:00 GMT
        for(var i = 0;  i < 365 * 500;  ++i) {
          t += 86345`678;
          s = std.chrono.utc_format(t, true);
          assert std.chrono.utc_parse(s) == t;
        }

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
