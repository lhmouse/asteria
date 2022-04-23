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

    var defer_throw = false;
    var call_throw = false;

    var ptc;
    var st1, st2;

    // convert the backtrace to somethign comparable
    func transform_backtrace(st, bt, ln) {
      st = [ ];
      // ignore top level calls
      for(each k, v -> bt)
        st[$] = [ v.frame, v.line >= ln - 3 ? 12345 : v.line ];
      // print the backtrace
      std.debug.dump(bt);
      return ->st;
    }

    func deferred(s) {
      if(defer_throw)
        throw s;
      else
        return 0;
    }
    func boom(...) {
      if(call_throw)
        throw 42;
      else
        return 0;
    }
    func bar() {
      defer deferred("third");
      defer deferred("fourth");
      if(ptc) boom();  else return 1+boom();  // keep this in a single line!
    }
    func foo() {
      defer deferred("first");
      defer deferred("second");
      if(ptc) bar();  else return 1+bar();  // keep this in a single line!
    }

    defer_throw = false;
    call_throw = false;
    // non-ptc
    ptc = false;
    try
      foo();
    catch(e)
      transform_backtrace(->st1, __backtrace, __line);
    // ptc
    ptc = true;
    try
      foo();
    catch(e)
      transform_backtrace(->st2, __backtrace, __line);
    // compare
    assert st1 == st2;

    defer_throw = true;
    call_throw = false;
    // non-ptc
    ptc = false;
    try
      foo();
    catch(e)
      transform_backtrace(->st1, __backtrace, __line);
    // ptc
    ptc = true;
    try
      foo();
    catch(e)
      transform_backtrace(->st2, __backtrace, __line);
    // compare
    assert st1 == st2;

    defer_throw = false;
    call_throw = true;
    // non-ptc
    ptc = false;
    try
      foo();
    catch(e)
      transform_backtrace(->st1, __backtrace, __line);
    // ptc
    ptc = true;
    try
      foo();
    catch(e)
      transform_backtrace(->st2, __backtrace, __line);
    // compare
    assert st1 == st2;

    defer_throw = true;
    call_throw = true;
    // non-ptc
    ptc = false;
    try
      foo();
    catch(e)
      transform_backtrace(->st1, __backtrace, __line);
    // ptc
    ptc = true;
    try
      foo();
    catch(e)
      transform_backtrace(->st2, __backtrace, __line);
    // compare
    assert st1 == st2;

///////////////////////////////////////////////////////////////////////////////
      )__"));
    code.execute();
  }
