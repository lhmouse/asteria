// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "util.hpp"
#include "../src/util.hpp"

using namespace asteria;

int main()
  {
    try {
      ASTERIA_THROW("test $1 $2 $$/end", "exception:", 42);
      ::std::terminate();
    }
    catch(exception& e) {
      ASTERIA_TEST_CHECK(::std::strstr(e.what(), "test exception: 42 $/end") != nullptr);
    }
  }
