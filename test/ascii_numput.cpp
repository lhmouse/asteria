// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../rocket/ascii_numput.hpp"
#include <float.h>
#include <math.h>
using namespace ::rocket;

static const struct {
  double input;
  const char* expected;
} cases[] = {
    // https://github.com/vitaut/zmij/blob/main/zmij-test.cc
    {6.62607015e-34, "6.62607015e-34"},
    {0, "0"},
    {-0.0, "-0"},
    {std::numeric_limits<double>::infinity(), "infinity"},
    {-std::numeric_limits<double>::infinity(), "-infinity"},
    {-4.932096661796888e-226, "-4.932096661796888e-226"},
    {3.439070283483335e+35, "3.439070283483335e+35"},
    {6.606854224493745e-17, "6.606854224493745e-17"},
    {6.079537928711555e+61, "6.079537928711555e+61"},
};

int main()
  {
    for(const auto& r : cases) {
      ::printf("---------------> checking: %.17g\n", r.input);
      ::printf("--------------->   exp: %s\n", r.expected);

      ascii_numput nump;
      nump.put_DD(r.input);
      ::printf("--------------->   got: %s\n", nump.data());
      ASTERIA_TEST_CHECK(::strcmp(nump.data(), r.expected) == 0);
    }

    ascii_numput np1;
    np1.set_radix_point('^');
    np1.put_DD(123456789012345678.9);
    ASTERIA_TEST_CHECK(::strcmp(np1.data(), "1^2345678901234568e+17") == 0);
    np1.put_DD(0.1234567890123456789);
    ASTERIA_TEST_CHECK(::strcmp(np1.data(), "0^12345678901234568") == 0);
    np1.put_DD(0.00000000000000000001234567890123456789);
    ASTERIA_TEST_CHECK(::strcmp(np1.data(), "1^2345678901234569e-20") == 0);
  }
