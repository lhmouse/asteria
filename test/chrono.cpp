// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/simple_script.hpp"
using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        assert std.chrono.format(std.numeric.integer_min) == "0000-01-01 00:00:00 UTC";  // min
        assert std.chrono.format(std.numeric.integer_min, true) == "0000-01-01 00:00:00.000 UTC";  // min

        assert std.chrono.format(std.numeric.integer_max) == "9999-01-01 00:00:00 UTC";  // max
        assert std.chrono.format(std.numeric.integer_max, true) == "9999-01-01 00:00:00.000 UTC";  // max

        assert std.chrono.format(0, false, 0) == "1970-01-01 00:00:00 UTC";
        assert std.chrono.format(0, true, 0) == "1970-01-01 00:00:00.000 UTC";

        assert std.chrono.parse("0000-01-01 00:00:00 UTC") == std.numeric.integer_min;
        assert std.chrono.parse("9999-01-01 00:00:00 UTC") == std.numeric.integer_max;

        var now = std.chrono.now();
        assert now > 0;
        std.io.putfln("now ---> $1", now);

        var now_str_utc = std.chrono.format(now, true, 0);
        std.io.putfln("now_str_utc ---> $1", now_str_utc);
        assert std.chrono.parse(now_str_utc) == now;

        var now_str_local = std.chrono.format(now, true);
        std.io.putfln("now_str_local ---> $1", now_str_local);
        assert std.chrono.parse(now_str_local) == now;

        var utc_off = std.chrono.parse(now_str_utc >> 4) - now;  // remove " UTC"
        std.io.putfln("utc_off ---> $1", utc_off);

        var t = 631152000000;  // 1990-01-01 00:00:00 UTC
        for(var i = 0;  i < 365 * 500;  ++i) {
          t += 86345`678;
          assert std.chrono.parse(std.chrono.format(t, true)) == t;
        }

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
