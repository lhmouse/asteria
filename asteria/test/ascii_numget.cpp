// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "utils.hpp"
#include "asteria/rocket/ascii_numget.hpp"

using namespace ::asteria;

// Test cases of known-bad strtod conversions that motivated the use of dmg_fp.
// See https://bugs.chromium.org/p/chromium/issues/detail?id=593512.

static const struct {
  const char* input;
  uint64_t expected;
} cases[] = {
    // http://www.exploringbinary.com/incorrectly-rounded-conversions-in-visual-c-plus-plus/
    {"9214843084008499", 0x43405e6cec57761aULL},
    {"0.500000000000000166533453693773481063544750213623046875",
     0x3fe0000000000002ULL},
    {"30078505129381147446200", 0x44997a3c7271b021ULL},
    {"1777820000000000000001", 0x4458180d5bad2e3eULL},
    {"0.500000000000000166547006220929549868969843373633921146392822265625",
     0x3fe0000000000002ULL},
    {"0.50000000000000016656055874808561867439493653364479541778564453125",
     0x3fe0000000000002ULL},
    {"0.3932922657273", 0x3fd92bb352c4623aULL},

    // http://www.exploringbinary.com/incorrectly-rounded-conversions-in-gcc-and-glibc/
    {"0.500000000000000166533453693773481063544750213623046875",
     0x3fe0000000000002ULL},
    {"3.518437208883201171875e13", 0x42c0000000000002ULL},
    {"62.5364939768271845828", 0x404f44abd5aa7ca4ULL},
    {"8.10109172351e-10", 0x3e0bd5cbaef0fd0cULL},
    {"1.50000000000000011102230246251565404236316680908203125",
     0x3ff8000000000000ULL},
    {"9007199254740991.4999999999999999999999999999999995",
     0x433fffffffffffffULL},

    // http://www.exploringbinary.com/incorrect-decimal-to-floating-point-conversion-in-sqlite/
    {"1e-23", 0x3b282db34012b251ULL},
    {"8.533e+68", 0x4e3fa69165a8eea2ULL},
    {"4.1006e-184", 0x19dbe0d1c7ea60c9ULL},
    {"9.998e+307", 0x7fe1cc0a350ca87bULL},
    {"9.9538452227e-280", 0x0602117ae45cde43ULL},
    {"6.47660115e-260", 0x0a1fdd9e333badadULL},
    {"7.4e+47", 0x49e033d7eca0adefULL},
    {"5.92e+48", 0x4a1033d7eca0adefULL},
    {"7.35e+66", 0x4dd172b70eababa9ULL},
    {"8.32116e+55", 0x4b8b2628393e02cdULL},

    // https://www.exploringbinary.com/php-hangs-on-numeric-value-2-2250738585072011e-308/
    {"2.2250738585072011e-308", 0x000fffffffffffffULL},
    {"2.2250738585072012e-308", 0x0010000000000000ULL},
};

int main()
  {
    for(const auto& r : cases) {
      ::printf("---------------> checking: %s\n", r.input);

      double exp;
      ::std::memcpy(&exp, &r.expected, sizeof(double));
      ::printf("--------------->   exp: %.17g\n", exp);

      const char* p = r.input;
      const char* e = p + ::std::strlen(p);

      ::rocket::ascii_numget numg;
      ASTERIA_TEST_CHECK(numg.parse_F(p, e));
      ASTERIA_TEST_CHECK(p == e);

      double val;
      ASTERIA_TEST_CHECK(numg.cast_F(val, -HUGE_VAL, +HUGE_VAL, false));
      uint64_t bits;
      ::std::memcpy(&bits, &val, sizeof(double));
      int err = static_cast<int>(bits - r.expected);

      ::printf("--------------->   got: %.17g (%+d ULP)\n\n", val, err);
      ASTERIA_TEST_CHECK(::std::abs(err) <= 1);
      // TODO: ASTERIA_TEST_CHECK(err == 0);
    }
  }
