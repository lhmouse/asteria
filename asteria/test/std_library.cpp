// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../src/compiler/simple_source_file.hpp"
#include "../src/runtime/global_context.hpp"
#include "../src/rocket/insertable_istream.hpp"

using namespace Asteria;

int main()
  {
    rocket::insertable_istream iss(
      rocket::sref(
        R"__(
          return std.meow;
        )__")
      );
    Simple_Source_File code(iss, rocket::sref("my_file"));
    Global_Context global;
    auto retv = code.execute(global, { }).read();
    ASTERIA_TEST_CHECK(retv.is_null());

    global.open_std_member(rocket::sref("meow")) = G_integer(42);
    retv = code.execute(global, { }).read();
    ASTERIA_TEST_CHECK(retv.as_integer() == 42);

    global.remove_std_member(rocket::sref("meow"));
    retv = code.execute(global, { }).read();
    ASTERIA_TEST_CHECK(retv.is_null());
  }
