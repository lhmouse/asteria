// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../asteria/src/runtime/traceable_exception.hpp"

using namespace Asteria;

int main()
  {
    try {
      Traceable_Exception except(D_integer(42), Source_Location(rocket::sref("file1"), 1), rocket::sref("func1"));
      except.append_frame(Source_Location(rocket::sref("file2"), 2), rocket::sref("func2"));
      except.append_frame(Source_Location(rocket::sref("file3"), 3), rocket::sref("func3"));
      throw except;
    } catch(Traceable_Exception& e) {
      ASTERIA_TEST_CHECK(e.get_value().check<D_integer>() == 42);
      ASTERIA_TEST_CHECK(e.get_frame_count() == 3);
      ASTERIA_TEST_CHECK(e.get_frame(0).source_file() ==        "file1");
      ASTERIA_TEST_CHECK(e.get_frame(0).source_line() ==             1 );
      ASTERIA_TEST_CHECK(e.get_frame(0).function_signature() == "func1");
      ASTERIA_TEST_CHECK(e.get_frame(1).source_file() ==        "file2");
      ASTERIA_TEST_CHECK(e.get_frame(1).source_line() ==             2 );
      ASTERIA_TEST_CHECK(e.get_frame(1).function_signature() == "func2");
      ASTERIA_TEST_CHECK(e.get_frame(2).source_file() ==        "file3");
      ASTERIA_TEST_CHECK(e.get_frame(2).source_line() ==             3 );
      ASTERIA_TEST_CHECK(e.get_frame(2).function_signature() == "func3");
    }
  }
