// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "utilities.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(::rocket::sref(
      R"__(
        assert std.system.execute('true') == 0;
        assert std.system.execute('false') != 0;

        try {
          // note this may or may not throw
          var status = std.system.execute('./nonexistent-command');
          assert status != 0;
        }
        catch(e)
          assert std.string.find(e, "assertion failure") == null;

        assert std.system.execute('bash',
          [ '-c', 'kill -1 $$' ]) == 129;
        assert std.system.execute('bash',
          [ '-c', 'kill -9 $$' ]) == 137;

        assert std.system.execute('bash',
          [ '-c', 'test $VAR == yes' ], [ 'VAR=yes' ]) == 0;
        assert std.system.execute('bash',
          [ '-c', 'test $VAR == yes' ], [ 'VAR=no' ]) != 0;

      )__"), tinybuf::open_read);

    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    code.execute(global);
  }
