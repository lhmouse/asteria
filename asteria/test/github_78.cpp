// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../src/simple_script.hpp"
#include "../src/runtime/global_context.hpp"

using namespace asteria;

int main()
  {
    Simple_Script code;
    code.reload_string(
      sref(__FILE__), __LINE__, sref(R"__(
///////////////////////////////////////////////////////////////////////////////

        func three() {
          func two() {
            func one() {
              return typeof two;
            }
            return one();
          }
          return two();
        }
        return three();

///////////////////////////////////////////////////////////////////////////////
      )__"));
    Global_Context global;
    ASTERIA_TEST_CHECK(code.execute(global).dereference_readonly().as_string() == "function");
  }
