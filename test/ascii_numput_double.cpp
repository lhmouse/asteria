// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../rocket/ascii_numput.hpp"
#include "../rocket/ascii_numget.hpp"
using namespace ::asteria;

int main()
  {
    ::rocket::ascii_numput nump;
    ::rocket::ascii_numget numg;
    char* eptr;
    double value, t;

    // go up to infinity
    value = 1234567890123456789.0;

    for(;;) {
      // decimal plain
      nump.put_DD(value);
      ASTERIA_TEST_CHECK(::strtod(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // decimal scientific
      nump.put_DED(value);
      ASTERIA_TEST_CHECK(::strtod(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // hex plain
      nump.put_XD(value);
      ASTERIA_TEST_CHECK(::strtod(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // hex scientific
      nump.put_XED(value);
      ASTERIA_TEST_CHECK(::strtod(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // binary plain
      nump.put_XD(value);
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // binary scientific
      nump.put_XED(value);
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      if(value == HUGE_VAL)
        break;

      value *= 11;
    }

    // go down to zero
    value = -1234567890123456789.0;

    for(;;) {
      // decimal plain
      nump.put_DD(value);
      ASTERIA_TEST_CHECK(::strtod(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // decimal scientific
      nump.put_DED(value);
      ASTERIA_TEST_CHECK(::strtod(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // hex plain
      nump.put_XD(value);
      ASTERIA_TEST_CHECK(::strtod(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // hex scientific
      nump.put_XED(value);
      ASTERIA_TEST_CHECK(::strtod(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // binary plain
      nump.put_XD(value);
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // binary scientific
      nump.put_XED(value);
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      if(value == 0)
        break;

      value /= 11;
    }
  }
