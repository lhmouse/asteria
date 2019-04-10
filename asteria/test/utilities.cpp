// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "test_utilities.hpp"
#include "../asteria/src/utilities.hpp"

using namespace Asteria;

int main()
  {
    try {
      ASTERIA_THROW_RUNTIME_ERROR("test", ' ', "exception: ", 42, '$');
      std::terminate();
    } catch(std::exception& e) {
      ASTERIA_TEST_CHECK(std::strstr(e.what(), "test exception: 42$") != nullptr);
    }
  }
