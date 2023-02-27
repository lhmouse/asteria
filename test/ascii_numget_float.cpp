// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "../rocket/ascii_numget.hpp"
#include <float.h>
using namespace ::asteria;

struct
  {
    float value;
    const char text[72];
    bool overflowed;
    bool underflowed;
    bool inexact;
  }
constexpr cases[] =
  {
    { HUGE_VALF,  "0x1p128",          1, 0, 0 }, { HUGE_VALF,  "3.40282367e+38", 1, 0, 1 },  // overflowing, inf
    { HUGE_VALF,  "0x1.ffffffp+127",  1, 0, 1 }, { HUGE_VALF,  "3.40282357e+38", 1, 0, 1 },  // overflowing, inf
    { FLT_MAX,    "0x1.fffffe8p+127", 0, 0, 1 }, { FLT_MAX,    "3.40282352e+38", 0, 0, 1 },  // truncated, max
    { FLT_MAX,    "0x1.fffffep+127",  0, 0, 0 }, { FLT_MAX,    "3.40282347e+38", 0, 0, 1 },  // exact, max
    { 0x1p127F,   "0x1p127",          0, 0, 0 }, { 0x1p127F,   "1.70141183e+38", 0, 0, 1 },  // exact, normal
    { 1.2578125F, "0x1.42p0",         0, 0, 0 }, { 1.2578125F, "1.2578125",      0, 0, 0 },  // exact, normal
    { 0x1.d6f346p+29F, "0x1.d6f346p+29", 0, 0, 0 }, { 0x1.d6f346p+29F, "987654321", 0, 0, 1 },  // normal
    { 0x1.e240cap+19F, "0x1.e240cap+19", 0, 0, 0 }, { 0x1.e240cap+19F, "987654.321", 0, 0, 1 },  // normal
    { FLT_MIN,    "0x1.000001p-126",  0, 0, 1 }, { FLT_MIN,    "1.17549442e-38", 0, 0, 1 },  // truncated, min normal
    { FLT_MIN,    "0x1p-126",         0, 0, 0 }, { FLT_MIN,    "1.17549435e-38", 0, 0, 1 },  // exact, min normal
    { FLT_MIN,    "0x0.ffffffp-126",  0, 0, 1 }, { FLT_MIN,    "1.17549429e-38", 0, 0, 1 },  // ceiled, min normal
    { 0x1.fffffcp-127F, "0x0.fffffep-126", 0, 0, 0 }, { 0x1.fffffcp-127F, "1.17549421e-38", 0, 0, 1 },  // exact, max subnormal
    { 0x1p-148F,  "0x1.8p-149",       0, 0, 1 }, { 0x1p-148F,  "2.80259693e-45", 0, 0, 1 },  // ceiled, subnormal
    { 0x1p-149F,  "0x1.4p-149",       0, 0, 1 }, { 0x1p-149F,  "1.75162308e-45", 0, 0, 1 },  // truncated, min subnormal
    { 0x1p-149F,  "0x1p-149",         0, 0, 0 }, { 0x1p-149F,  "1.40129846e-45", 0, 0, 1 },  // exact, min subnormal
    { 0,          "0x0.8p-149",       0, 1, 1 }, { 0,          "7.00649232e-46", 0, 1, 1 },  // underflowed, zero
    { 0,          "0x0.4p-149",       0, 1, 1 }, { 0,          "3.50324616e-46", 0, 1, 1 },  // underflowed, zero
  };

int main()
  {
    for(const auto& r : cases) {
      ::printf("-------> checking: text = %s\n", r.text);

      ASTERIA_TEST_CHECK(::std::strtof(r.text, nullptr) == r.value);

      ::rocket::ascii_numget numg;
      ASTERIA_TEST_CHECK(numg.parse_D(r.text, ::strlen(r.text)) == ::strlen(r.text));

      float result;
      numg.cast_F(result, -HUGE_VALF, HUGE_VALF);

      ::printf("  value  = %a, ovfl = %d, udfl = %d, inxct = %d\n",
                  (double) r.value, r.overflowed, r.underflowed, r.inexact);

      ::printf("  result = %a, ovfl = %d, udfl = %d, inxct = %d\n",
                  (double) result, numg.overflowed(), numg.underflowed(), numg.inexact());

      ASTERIA_TEST_CHECK(::memcmp(&result, &r.value, sizeof(float)) == 0);
      ASTERIA_TEST_CHECK(numg.overflowed() == r.overflowed);
      ASTERIA_TEST_CHECK(numg.underflowed() == r.underflowed);
      ASTERIA_TEST_CHECK(numg.inexact() == r.inexact);
    }
  }
