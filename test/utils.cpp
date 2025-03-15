// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../asteria/utils.hpp"
using namespace ::asteria;

int main()
  {
    try {
      ASTERIA_THROW((
          "test $1 $2 $$/end"),
          "exception:", 42);
    }
    catch(exception& e) {
      ASTERIA_TEST_CHECK(::std::strstr(e.what(),
          "test exception: 42 $/end") != nullptr);
    }
  }
