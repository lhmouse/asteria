// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/runtime/traceable_exception.hpp"

using namespace Asteria;

int main()
  {
    try {
      Traceable_Exception except(D_integer(42));
      except.append_frame(Source_Location(rocket::sref("myfile"), 123));
      throw except;
    } catch(Traceable_Exception &e) {
      ASTERIA_TEST_CHECK(e.get_value().check<D_integer>() == 42);
      ASTERIA_TEST_CHECK(e.get_frame_count() == 1);
      ASTERIA_TEST_CHECK(e.get_frame(0).file() == "myfile");
      ASTERIA_TEST_CHECK(e.get_frame(0).line() == 123);
    }
  }
