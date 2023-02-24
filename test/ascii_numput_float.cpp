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
    float value, t;

    // go up to infinity
    value = 1234567890123456789.0f;

    for(;;) {
      // decimal plain
      nump.put_DF(value);
      ASTERIA_TEST_CHECK(::strtof(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // decimal scientific
      nump.put_DEF(value);
      ASTERIA_TEST_CHECK(::strtof(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // hex plain
      nump.put_XF(value);
      ASTERIA_TEST_CHECK(::strtof(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // hex scientific
      nump.put_XEF(value);
      ASTERIA_TEST_CHECK(::strtof(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // binary plain
      nump.put_XF(value);
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // binary scientific
      nump.put_XEF(value);
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      if(value == HUGE_VALF)
        break;

      value *= 11;
    }

    // go down to zero
    value = -1234567890123456789.0f;

    for(;;) {
      // decimal plain
      nump.put_DF(value);
      ASTERIA_TEST_CHECK(::strtof(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // decimal scientific
      nump.put_DEF(value);
      ASTERIA_TEST_CHECK(::strtof(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // hex plain
      nump.put_XF(value);
      ASTERIA_TEST_CHECK(::strtof(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // hex scientific
      nump.put_XEF(value);
      ASTERIA_TEST_CHECK(::strtof(nump.begin(), &eptr) == value);
      ASTERIA_TEST_CHECK(eptr == nump.end());
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // binary plain
      nump.put_XF(value);
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      // binary scientific
      nump.put_XEF(value);
      numg.get(t, nump.begin(), nump.size());
      ASTERIA_TEST_CHECK(t == value);

      if(value == 0)
        break;

      value /= 11;
    }
  }
