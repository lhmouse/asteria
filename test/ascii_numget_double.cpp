// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../rocket/ascii_numget.hpp"
#include <float.h>
using namespace ::asteria;

struct
  {
    double value;
    const char text[72];
    bool overflowed;
    bool underflowed;
    bool inexact;
  }
constexpr cases[] =
  {
    { HUGE_VAL,  "0x1p1024",                 1, 0, 0 }, { HUGE_VAL,  "1.79769313486231591e+308", 1, 0, 1 },  // overflowing, inf
    { HUGE_VAL,  "0xf.ffffffffffffcp+1020",  1, 0, 1 }, { HUGE_VAL,  "1.79769313486231581e+308", 1, 0, 1 },  // overflowing, inf
    { DBL_MAX,   "0xf.ffffffffffffap+1020",  0, 0, 1 }, { DBL_MAX,   "1.79769313486231576e+308", 0, 0, 1 },  // truncated, max
    { DBL_MAX,   "0x1.fffffffffffffp+1023",  0, 0, 0 }, { DBL_MAX,   "1.79769313486231571e+308", 0, 0, 1 },  // exact, max
    { 0x1p127,   "0x1p127",                  0, 0, 0 }, { 0x1p127,   "1.701411834604692320e+38", 0, 0, 1 },  // exact, normal
    { 1.2578125, "0xa.1p-3",                 0, 0, 0 }, { 1.2578125, "1.2578125",                0, 0, 0 },  // exact, normal
    { 0x1.12210f716068ep+63, "0x1.12210f716068ep+63", 0, 0, 0 }, { 0x1.12210f716068ep+63, "9876543210123456789", 0, 0, 1 },  // normal
    { 0x1.674e79fc65ca4p+46, "0x1.674e79fc65ca4p+46", 0, 0, 0 }, { 0x1.674e79fc65ca4p+46, "98765432101234.56789", 0, 0, 1 },  // normal
    { DBL_MIN,   "0x1.00000000000008p-1022", 0, 0, 1 }, { DBL_MIN,   "2.22507385850720163e-308", 0, 0, 1 },  // truncated, min normal
    { DBL_MIN,   "0x1p-1022",                0, 0, 0 }, { DBL_MIN,   "2.22507385850720138e-308", 0, 0, 1 },  // exact, min normal
    { DBL_MIN,   "0x0.fffffffffffff8p-1022", 0, 0, 1 }, { DBL_MIN,   "2.22507385850720114e-308", 0, 0, 1 },  // ceiled, min normal
    { 0xf.ffffffffffffp-1026, "0x0.fffffffffffffp-1022", 0, 0, 0 }, { 0xf.ffffffffffffp-1026, "2.22507385850720089e-308", 0, 0, 1 },  // exact, max subnormal
    { 0x1p-1073, "0x1.8p-1074",              0, 0, 1 }, { 0x1p-1073, "7.41098468761869817e-324", 0, 0, 1 },  // ceiled, subnormal
    { 0x1p-1074, "0x1.4p-1074",              0, 0, 1 }, { 0x1p-1074, "6.17582057301558180e-324", 0, 0, 1 },  // truncated, min subnormal
    { 0x1p-1074, "0x1p-1074",                0, 0, 0 }, { 0x1p-1074, "4.94065645841246544e-324", 0, 0, 1 },  // exact, min subnormal
    { 0,         "0x0.8p-1074",              0, 1, 1 }, { 0,         "2.47032822920623272e-324", 0, 1, 1 },  // underflowed, zero
    { 0,         "0x0.4p-1074",              0, 1, 1 }, { 0,         "1.23516411460311636e-324", 0, 1, 1 },  // underflowed, zero
  };

int main()
  {
    for(const auto& r : cases) {
      ::printf("-------> checking: text = %s\n", r.text);

      ASTERIA_TEST_CHECK(::std::strtod(r.text, nullptr) == r.value);

      ::rocket::ascii_numget numg;
      ASTERIA_TEST_CHECK(numg.parse_D(r.text, ::strlen(r.text)) == ::strlen(r.text));

      double result = 12345;
      numg.cast_D(result, -HUGE_VAL, HUGE_VAL);

      ::printf("  value  = %a, ovfl = %d, udfl = %d, inxct = %d\n",
                  r.value, r.overflowed, r.underflowed, r.inexact);

      ::printf("  result = %a, ovfl = %d, udfl = %d, inxct = %d\n",
                  result, numg.overflowed(), numg.underflowed(), numg.inexact());

      ASTERIA_TEST_CHECK(::memcmp(&result, &r.value, sizeof(double)) == 0);
      ASTERIA_TEST_CHECK(numg.overflowed() == r.overflowed);
      ASTERIA_TEST_CHECK(numg.underflowed() == r.underflowed);
      ASTERIA_TEST_CHECK(numg.inexact() == r.inexact);
    }
  }
