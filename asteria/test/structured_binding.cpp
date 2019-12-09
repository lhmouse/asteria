// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    rocket::tinybuf_str cbuf;
    cbuf.set_string(rocket::sref(
      R"__(
        func make_array() {
          return [ 42, "hello", true ];
        }
        func make_object() {
          return { at: "@", dollar: "$" };
        }

        {
          var [ a, b, c ] = make_array();
          assert a == 42;
          assert b == "hello";
          assert c == true;
        }

        {
          var [ d, c, b, a ] = make_array();
          assert d == 42;
          assert c == "hello";
          assert b == true;
          assert a == null;
        }

        {
          var [ c ] = make_array();
          assert c == 42;
        }

        {
          var { at, dollar } = make_object();
          assert at == "@";
          assert dollar == "$";
        }

        {
          var { at, nonexistent, dollar } = make_object();
          assert at == "@";
          assert nonexistent == null;
          assert dollar == "$";
        }

        {
          var { at }  = make_object();
          assert at == "@";
        }

        {
          var { nonexistent_again } = make_object();
          assert nonexistent_again == null;
        }
      )__"), tinybuf::open_read);
    Simple_Script code(cbuf, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global);
  }
