// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "util.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      ::rocket::sref(__FILE__), __LINE__, ::rocket::sref(R"__(
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

        func xpush(x) {
          rec[$] = x;
          throw x;
        }

        func foo() {
          defer xpush(1);
          defer xpush(2);
          defer xpush(3);
          throw 4;
        }
        rec = [ ];
        try
          foo();
        catch(e) {
          assert e == 1;
          assert rec == [3,2,1];
        }

///////////////////////////////////////////////////////////////////////////////
      )__"));
    Global_Context global;
    code.execute(global);
  }
