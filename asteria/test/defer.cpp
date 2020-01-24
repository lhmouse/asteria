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
///////////////////////////////////////////////////////////////////////////////

        var rec;

        func foo() {
          defer rec[$] = 'a';
          defer rec[$] = 'b';
          defer rec[$] = 'c';
        }
        rec = [ ];
        foo();
        assert rec == ['c','b','a'];

        func foo() {
          defer rec[$] = 1;
          defer rec[$] = false;
          defer rec[$] = infinity;
          throw 42;
        }
        rec = [ ];
        try
          foo();
        catch(e) {
          assert e == 42;
          assert rec == [infinity,false,1];
        }

        func foo() {
          defer rec[$] = 1;
          func bar() {
            defer rec[$] = 2;
            return "hello";
          }
          return bar() + " world!";  // non-ptc
        }
        rec = [ ];
        assert foo() == "hello world!";
        assert rec == [2,1];

        func foo() {
          defer rec[$] = 1;
          func bar() {
            defer rec[$] = 2;
            throw "hello";
          }
          return bar() + " world!";  // non-ptc
        }
        rec = [ ];
        try
          foo();
        catch(e) {
          assert e == "hello";
          assert rec == [2,1];
        }

        func foo() {
          defer rec[$] = 1;
          func bar() {
            defer rec[$] = 2;
            return "hello";
          }
          return bar();  // ptc
        }
        rec = [ ];
        assert foo() == "hello";
        assert rec == [2,1];

        func foo() {
          defer rec[$] = 1;
          func bar() {
            defer rec[$] = 2;
            throw "hello";
          }
          return bar();  // ptc
        }
        rec = [ ];
        try
          foo();
        catch(e) {
          assert e == "hello";
          assert rec == [2,1];
        }

///////////////////////////////////////////////////////////////////////////////
      )__"), tinybuf::open_read);
    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    code.execute(global);
  }
