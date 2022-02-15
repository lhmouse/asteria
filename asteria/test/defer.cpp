// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"

using namespace ::asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        func foo() {
          var i = 42;
          defer "meow";
          defer ++i;
          return i;  // by val
        }
        assert foo() == 42;

        func foo() {
          var i = 42;
          defer "meow";
          defer ++i;
          return ref i;  // by ref
        }
        assert foo() == 43;

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
    code.execute();
  }
