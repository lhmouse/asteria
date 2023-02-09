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

        std.debug.logf("__file = $1", __file);
        const path = std.string.pcre_replace(__file, '/csv\.cpp$', '/test.csv');
        const rows = std.csv.parse_file(path);

        assert countof rows == 5;
        assert rows[0] == [ '1', '2', '3' ];
        assert rows[1] == [ 'value', 'has', 'a"double', 'quote' ];
        assert rows[2] == [ 'and', 'escaped"' ];
        assert rows[3] == [ 'a', 'nested', "line\nbreak", 'is', 'acceptible' ];
        assert rows[4] == [ 'a bc"d e"', '4' ];

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
