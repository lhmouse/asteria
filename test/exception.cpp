// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../src/exception.hpp"

using namespace Asteria;

int main()
  {
    try {
      throw Exception(D_integer(42));
    } catch(Exception &e) {
      ASTERIA_TEST_CHECK(e.get_value().check<D_integer>() == 42);
    }
  }
