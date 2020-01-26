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

    var defer_throw = false;
    var call_throw = false;

    var ptc;
    var st1, st2;

    // convert the backtrace to somethign comparable
    func transform_backtrace(st, bt) {
      st = [ ];
      // discard the last call site
      const k = lengthof(bt) - 2;
      for(var i = 0; i < k; ++i) {
        st[$] = bt[i].frame;
        st[$] = bt[i].line;
        st[$] = bt[i].value;
      }
      // print the backtrace
      std.debug.print("backtrace: $1", st);
      return& st;
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
      transform_backtrace(&st1, __backtrace);
    // ptc
    ptc = true;
    try
      foo();
    catch(e)
      transform_backtrace(&st2, __backtrace);
    // compare
    assert st1 == st2;

    defer_throw = true;
    call_throw = false;
    // non-ptc
    ptc = false;
    try
      foo();
    catch(e)
      transform_backtrace(&st1, __backtrace);
    // ptc
    ptc = true;
    try
      foo();
    catch(e)
      transform_backtrace(&st2, __backtrace);
    // compare
    assert st1 == st2;

    defer_throw = false;
    call_throw = true;
    // non-ptc
    ptc = false;
    try
      foo();
    catch(e)
      transform_backtrace(&st1, __backtrace);
    // ptc
    ptc = true;
    try
      foo();
    catch(e)
      transform_backtrace(&st2, __backtrace);
    // compare
    assert st1 == st2;

    defer_throw = true;
    call_throw = true;
    // non-ptc
    ptc = false;
    try
      foo();
    catch(e)
      transform_backtrace(&st1, __backtrace);
    // ptc
    ptc = true;
    try
      foo();
    catch(e)
      transform_backtrace(&st2, __backtrace);
    // compare
    assert st1 == st2;

///////////////////////////////////////////////////////////////////////////////
      )__"), tinybuf::open_read);
    Simple_Script code(cbuf, ::rocket::sref(__FILE__));
    Global_Context global;
    code.execute(global);
  }
