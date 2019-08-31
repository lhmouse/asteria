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
      )__";

    cow_isstream iss(rocket::sref(s_source));
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    code.execute(global, { });
  }
