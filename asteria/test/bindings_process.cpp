// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(::rocket::sref(
      R"__(
        assert std.process.execute('/bin/true') == 0;
        assert std.process.execute('/bin/false') != 0;

        try {
          std.process.execute('nonexistent-command');
          assert false;
        }
        catch(e)
          ;

        assert std.process.execute('/bin/bash',
          [ '/bin/bash', '-c', 'kill -1 $$' ]) == 129;
        assert std.process.execute('/bin/bash',
          [ '/bin/bash', '-c', 'kill -9 $$' ]) == 137;

        assert std.process.execute('/bin/bash',
          [ '/bin/bash', '-c', 'test $VAR == yes' ], [ 'VAR=yes' ]) == 0;
        assert std.process.execute('/bin/bash',
          [ '/bin/bash', '-c', 'test $VAR == yes' ], [ 'VAR=no' ]) != 0;

      )__"), tinybuf::open_read);

    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    code.execute(global);
  }
