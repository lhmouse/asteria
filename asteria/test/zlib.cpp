// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        const trailer = "\x00\x00\xff\xff";

        // https://datatracker.ietf.org/doc/html/rfc7692#section-7.2.3.2
        var defl = std.zlib.Deflator("raw");
        var infl = std.zlib.Inflator("raw");

        defl.update("Hello");
        defl.flush();
        assert defl.output == "\xf2\x48\xcd\xc9\xc9\x07\x00" + trailer;
        infl.update(defl.output);
        infl.flush();
        assert infl.output == "Hello";

        defl.output = "";
        defl.update("Hello");
        defl.flush();
        assert defl.output == "\xf2\x00\x11\x00\x00" + trailer;
        infl.update(defl.output);
        infl.flush();
        assert infl.output == "HelloHello";

        // https://datatracker.ietf.org/doc/html/rfc7692#section-7.2.3.3
        var defl = std.zlib.Deflator("raw", 0);
        var infl = std.zlib.Inflator("raw");
        defl.update("Hello");
        defl.flush();
        assert defl.output == "\x00\x05\x00\xfa\xff\x48\x65\x6c\x6c\x6f\x00" + trailer;
        infl.update(defl.output);
        infl.flush();
        assert infl.output == "Hello";

        // https://datatracker.ietf.org/doc/html/rfc7692#section-7.2.3.5
        var defl = std.zlib.Deflator("raw");
        var infl = std.zlib.Inflator("raw");
        defl.update("He");
        defl.flush();
        defl.update("llo");
        defl.flush();
        assert defl.output == "\xf2\x48\x05\x00\x00\x00\xff\xff\xca\xc9\xc9\x07\x00" + trailer;
        infl.update(defl.output);
        infl.flush();
        assert infl.output == "Hello";

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
