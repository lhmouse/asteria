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

        assert std.system.call('true') == 0;
        assert std.system.call('false') != 0;

        try {
          // note this may or may not throw
          var status = std.system.call('./nonexistent-command');
          assert status != 0;
        }
        catch(e)
          assert std.string.find(e, "assertion failure") == null;

        assert std.system.call('bash',
          [ '-c', 'kill -1 $$' ]) == 129;
        assert std.system.call('bash',
          [ '-c', 'kill -9 $$' ]) == 137;

        assert std.system.call('bash',
          [ '-c', 'test $VAR == yes' ], [ 'VAR=yes' ]) == 0;
        assert std.system.call('bash',
          [ '-c', 'test $VAR == yes' ], [ 'VAR=no' ]) != 0;

        assert std.system.pipe('true') == '';
        assert std.system.pipe('false') == null;

        try {
          // note this may or may not throw
          var out = std.system.pipe('./nonexistent-command');
          assert out == null;
        }
        catch(e)
          assert std.string.find(e, "assertion failure") == null;

        assert std.system.pipe('printf', [ '%s %d', 'hello', '123' ]) == "hello 123";
        assert std.system.pipe('tr', [ 'aeiou', '12345' ], 'abcdef') == '1bcd2f';

        var o = std.system.load_conf(std.string.pcre_replace(__file, '/[^/]*$', '/sample.conf'));
        assert o.key == "value";
        assert o["keys may be quoted"] == 'no need to escape \ in single quotes';
        assert o.utf_text == "\U01F602\U01F602";
        assert o.values.may == "be";
        assert o.values.objects == 42;
        assert o.and == ["can","also","be","arrays"];
        assert o.or == null;
        assert o.long_integers == 0x123456789ABCDEF;
        assert o.binary_integers == 0b10010010;
        assert o.hexadecimal_float == 0x1.23p-62;
        assert o.binary_float == 0b100.0110p3;

///////////////////////////////////////////////////////////////////////////////
      )__");
    code.execute();
  }
