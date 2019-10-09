// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace Asteria;

int main()
  {
    tinybuf_str sbuf;
    sbuf.set_string(rocket::sref(
      R"__(
        var one = 1;
        const two = 2;
        func fib(n) {
          return n <= one ? n : fib(n - one) + fib(n - two);
        }
        var con = { };
        con["value"] = fib(11);
        con["const"] = one;
        return con.value + con.const;
      )__"), tinybuf::open_read);
    Simple_Script code;
    code.reload(sbuf, rocket::sref("<test>"));
    Global_Context global;
    auto res = code.execute(global);
    ASTERIA_TEST_CHECK(res.read().as_integer() == 90);
  }
