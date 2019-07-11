// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/runtime/exception.hpp"

using namespace Asteria;

int main()
  {
    try {
      Exception except;
      except.open_value() = G_integer(42);
      except.push_frame(rocket::sref("file1"), 1, Backtrace_Frame::ftype_native, G_string(rocket::sref("func1")));
      except.push_frame(rocket::sref("file2"), 2, Backtrace_Frame::ftype_catch,  G_string(rocket::sref("func2")));
      except.push_frame(rocket::sref("file3"), 3, Backtrace_Frame::ftype_func,   G_string(rocket::sref("func3")));
      throw except;
    }
    catch(Exception& e) {
      ASTERIA_TEST_CHECK(e.get_value().as_integer() == 42);
      ASTERIA_TEST_CHECK(e.count_frames() == 3);
      ASTERIA_TEST_CHECK(e.get_frame(0).file() == "file1");
      ASTERIA_TEST_CHECK(e.get_frame(0).line() == 1);
      ASTERIA_TEST_CHECK(e.get_frame(0).ftype() == Backtrace_Frame::ftype_native);
      ASTERIA_TEST_CHECK(e.get_frame(0).value().as_string() == "func1");
      ASTERIA_TEST_CHECK(e.get_frame(1).file() == "file2");
      ASTERIA_TEST_CHECK(e.get_frame(1).line() == 2);
      ASTERIA_TEST_CHECK(e.get_frame(1).ftype() == Backtrace_Frame::ftype_catch);
      ASTERIA_TEST_CHECK(e.get_frame(1).value().as_string() == "func2");
      ASTERIA_TEST_CHECK(e.get_frame(2).file() == "file3");
      ASTERIA_TEST_CHECK(e.get_frame(2).line() == 3);
      ASTERIA_TEST_CHECK(e.get_frame(2).ftype() == Backtrace_Frame::ftype_func);
      ASTERIA_TEST_CHECK(e.get_frame(2).value().as_string() == "func3");
    }
  }
