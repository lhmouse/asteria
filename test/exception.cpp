// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/runtime/exception.hpp"

using namespace Asteria;

int main()
  {
    try {
      throw Exception(Source_location(std::ref("myfile"), 123), D_integer(42));
    } catch(Exception &e) {
      ASTERIA_TEST_CHECK(e.get_location().get_file() == "myfile");
      ASTERIA_TEST_CHECK(e.get_location().get_line() == 123);
      ASTERIA_TEST_CHECK(e.get_value().check<D_integer>() == 42);
    }
  }
