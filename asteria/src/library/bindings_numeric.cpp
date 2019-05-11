// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_numeric.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/collector.hpp"
#include "../utilities.hpp"

namespace Asteria {

G_integer std_numeric_abs(const G_integer& value)
  {
    if(value == INT64_MIN) {
      ASTERIA_THROW_RUNTIME_ERROR("The absolute value of `", value, "` cannot be represented as an `integer`.");
    }
    return std::abs(value);
  }

G_real std_numeric_abs(const G_real& value)
  {
    return std::fabs(value);
  }

G_integer std_numeric_sign(const G_integer& value)
  {
    return value >> 63;
  }

G_integer std_numeric_sign(const G_real& value)
  {
    return std::signbit(value) ? -1 : 0;
  }

G_boolean std_numeric_is_finite(const G_integer& /*value*/)
  {
    return true;
  }

G_boolean std_numeric_is_finite(const G_real& value)
  {
    return std::isfinite(value);
  }

G_boolean std_numeric_is_infinity(const G_integer& /*value*/)
  {
    return false;
  }

G_boolean std_numeric_is_infinity(const G_real& value)
  {
    return std::isinf(value);
  }

G_boolean std_numeric_is_nan(const G_integer& /*value*/)
  {
    return false;
  }

G_boolean std_numeric_is_nan(const G_real& value)
  {
    return std::isnan(value);
  }

    namespace {

    const G_integer& do_verify_bounds(const G_integer& lower, const G_integer& upper)
      {
        if(!(lower <= upper)) {
          ASTERIA_THROW_RUNTIME_ERROR("The `lower` bound must be less than or equal to the `upper` bound (got `", lower, "` and `", upper, "`).");
        }
        return upper;
      }
    const G_real& do_verify_bounds(const G_real& lower, const G_real& upper)
      {
        if(!std::islessequal(lower, upper)) {
          ASTERIA_THROW_RUNTIME_ERROR("The `lower` bound must be less than or equal to the `upper` bound (got `", lower, "` and `", upper, "`).");
        }
        return upper;
      }

    }

G_integer std_numeric_clamp(const G_integer& value, const G_integer& lower, const G_integer& upper)
  {
    return rocket::clamp(value, lower, do_verify_bounds(lower, upper));
  }

G_real std_numeric_clamp(const G_real& value, const G_real& lower, const G_real& upper)
  {
    return rocket::clamp(value, lower, do_verify_bounds(lower, upper));
  }

G_integer std_numeric_round(const G_integer& value)
  {
    return value;
  }

G_real std_numeric_round(const G_real& value)
  {
    return std::round(value);
  }

G_integer std_numeric_floor(const G_integer& value)
  {
    return value;
  }

G_real std_numeric_floor(const G_real& value)
  {
    return std::floor(value);
  }

G_integer std_numeric_ceil(const G_integer& value)
  {
    return value;
  }

G_real std_numeric_ceil(const G_real& value)
  {
    return std::ceil(value);
  }

G_integer std_numeric_trunc(const G_integer& value)
  {
    return value;
  }

G_real std_numeric_trunc(const G_real& value)
  {
    return std::trunc(value);
  }

    namespace {

    G_integer do_icast(const G_real& value)
      {
        if(!std::islessequal(INT64_MIN, value) || !std::islessequal(value, INT64_MAX)) {
          ASTERIA_THROW_RUNTIME_ERROR("The `real` number `", value, "` cannot be represented as an `integer`.");
        }
        return G_integer(value);
      }

    }

G_integer std_numeric_iround(const G_integer& value)
  {
    return value;
  }

G_integer std_numeric_iround(const G_real& value)
  {
    return do_icast(std::round(value));
  }

G_integer std_numeric_ifloor(const G_integer& value)
  {
    return value;
  }

G_integer std_numeric_ifloor(const G_real& value)
  {
    return do_icast(std::floor(value));
  }

G_integer std_numeric_iceil(const G_integer& value)
  {
    return value;
  }

G_integer std_numeric_iceil(const G_real& value)
  {
    return do_icast(std::ceil(value));
  }

G_integer std_numeric_itrunc(const G_integer& value)
  {
    return value;
  }

G_integer std_numeric_itrunc(const G_real& value)
  {
    return do_icast(std::trunc(value));
  }

G_real std_numeric_random(const Global_Context& global, const Opt<G_real>& limit)
  {
    if(limit) {
      int fpcls = std::fpclassify(*limit);
      if((fpcls == FP_INFINITE) || (fpcls == FP_NAN)) {
        ASTERIA_THROW_RUNTIME_ERROR("The `limit` for random numbers shall be finite (got `", *limit, "`).");
      }
      if(fpcls == FP_ZERO) {
        ASTERIA_THROW_RUNTIME_ERROR("The `limit` for random numbers shall not be zero (got `", *limit, "`).");
      }
    }
    // sqword <= [0,INT64_MAX]
    std::int64_t sqword = global.get_random_uint32();
    sqword <<= 31;
    sqword ^= global.get_random_uint32();
    // `sqword / 0x1p63` <= [0,1)
    return static_cast<double>(sqword) / 0x1p63 * limit.value_or(1);
  }

G_real std_numeric_sqrt(const G_real& x)
  {
    return std::sqrt(x);
  }

G_real std_numeric_fma(const G_real& x, const G_real& y, const G_real& z)
  {
    return std::fma(x, y, z);
  }

G_integer std_numeric_addm(const G_integer& x, const G_integer& y)
  {
    return G_integer(static_cast<std::uint64_t>(x) + static_cast<std::uint64_t>(y));
  }

G_integer std_numeric_subm(const G_integer& x, const G_integer& y)
  {
    return G_integer(static_cast<std::uint64_t>(x) - static_cast<std::uint64_t>(y));
  }

G_integer std_numeric_mulm(const G_integer& x, const G_integer& y)
  {
    return G_integer(static_cast<std::uint64_t>(x) * static_cast<std::uint64_t>(y));
  }

    namespace {

    ROCKET_PURE_FUNCTION G_integer do_saturing_add(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs >= 0) {
          if(lhs > INT64_MAX - rhs) {
            return INT64_MAX;
          }
        } else {
          if(lhs < INT64_MIN - rhs) {
            return INT64_MIN;
          }
        }
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_saturing_sub(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs >= 0) {
          if(lhs < INT64_MIN + rhs) {
            return INT64_MIN;
          }
        } else {
          if(lhs > INT64_MAX + rhs) {
            return INT64_MAX;
          }
        }
        return lhs - rhs;
      }

    ROCKET_PURE_FUNCTION G_integer do_saturing_mul(const G_integer& lhs, const G_integer& rhs)
      {
        if((lhs == 0) || (rhs == 0)) {
          return 0;
        }
        if((lhs == 1) || (rhs == 1)) {
          return (lhs ^ rhs) ^ 1;
        }
        if((lhs == INT64_MIN) || (rhs == INT64_MIN)) {
          return (lhs >> 63) ^ (rhs >> 63) ^ INT64_MAX;
        }
        if((lhs == -1) || (rhs == -1)) {
          return (lhs ^ rhs) + 1;
        }
        // signed lhs and absolute rhs
        auto slhs = lhs;
        auto arhs = rhs;
        if(rhs < 0) {
          slhs = -lhs;
          arhs = -rhs;
        }
        if(slhs >= 0) {
          if(slhs > INT64_MAX / arhs) {
            return INT64_MAX;
          }
        } else {
          if(slhs < INT64_MIN / arhs) {
            return INT64_MIN;
          }
        }
        return slhs * arhs;
      }

    ROCKET_PURE_FUNCTION G_real do_saturing_add(const G_real& lhs, const G_real& rhs)
      {
        return lhs + rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_saturing_sub(const G_real& lhs, const G_real& rhs)
      {
        return lhs - rhs;
      }

    ROCKET_PURE_FUNCTION G_real do_saturing_mul(const G_real& lhs, const G_real& rhs)
      {
        return lhs * rhs;
      }

    }

G_integer std_numeric_adds(const G_integer& x, const G_integer& y)
  {
    return do_saturing_add(x, y);
  }

G_real std_numeric_adds(const G_real& x, const G_real& y)
  {
    return do_saturing_add(x, y);
  }

G_integer std_numeric_adds(const G_integer& x, const G_integer& y, const G_integer& lower, const G_integer& upper)
  {
    return rocket::clamp(do_saturing_add(x, y), lower, do_verify_bounds(lower, upper));
  }

G_real std_numeric_adds(const G_real& x, const G_real& y, const G_real& lower, const G_real& upper)
  {
    return rocket::clamp(do_saturing_add(x, y), lower, do_verify_bounds(lower, upper));
  }

G_integer std_numeric_subs(const G_integer& x, const G_integer& y)
  {
    return do_saturing_sub(x, y);
  }

G_real std_numeric_subs(const G_real& x, const G_real& y)
  {
    return do_saturing_sub(x, y);
  }

G_integer std_numeric_subs(const G_integer& x, const G_integer& y, const G_integer& lower, const G_integer& upper)
  {
    return rocket::clamp(do_saturing_sub(x, y), lower, do_verify_bounds(lower, upper));
  }

G_real std_numeric_subs(const G_real& x, const G_real& y, const G_real& lower, const G_real& upper)
  {
    return rocket::clamp(do_saturing_sub(x, y), lower, do_verify_bounds(lower, upper));
  }

G_integer std_numeric_muls(const G_integer& x, const G_integer& y)
  {
    return do_saturing_mul(x, y);
  }

G_real std_numeric_muls(const G_real& x, const G_real& y)
  {
    return do_saturing_mul(x, y);
  }

G_integer std_numeric_muls(const G_integer& x, const G_integer& y, const G_integer& lower, const G_integer& upper)
  {
    return rocket::clamp(do_saturing_mul(x, y), lower, do_verify_bounds(lower, upper));
  }

G_real std_numeric_muls(const G_real& x, const G_real& y, const G_real& lower, const G_real& upper)
  {
    return rocket::clamp(do_saturing_mul(x, y), lower, do_verify_bounds(lower, upper));
  }

    namespace {

    constexpr char s_xdigits[] = "00112233445566778899aAbBcCdDeEfF";
    constexpr char s_spaces[] = " \f\n\r\t\v";

    }

G_string std_numeric_format(const G_integer& value, const Opt<G_integer>& base)
  {
    return { };
  }

G_string std_numeric_format(const G_real& value, const Opt<G_integer>& base)
  {
    return { };
  }

G_string std_numeric_format(const G_integer& value, const Opt<G_integer>& base, const G_integer& exp_base)
  {
    return { };
  }

G_string std_numeric_format(const G_real& value, const Opt<G_integer>& base, const G_integer& exp_base)
  {
    return { };
  }

    namespace {

    inline int do_compare_lowercase(const G_string& str, std::size_t from, const char* cmp, std::size_t len) noexcept
      {
        for(std::size_t si = from, ci = 0; ci != len; ++si, ++ci) {
          // Read a character from `str` and convert it into lowercase.
          int sc = static_cast<unsigned char>(str[si]);
          if(('A' <= sc) && (sc <= 'Z')) {
            sc |= 0x20;
          }
          // Compare it with the character from `cmp`, which must not be in uppercase.
          sc -= static_cast<unsigned char>(cmp[ci]);
          if(sc != 0) {
            return sc;
          }
        }
        // The strings are equal.
        return 0;
      }

    inline std::uint8_t do_translate_digit(char ch) noexcept
      {
        return static_cast<std::uint8_t>((std::find(s_xdigits, s_xdigits + 32, ch) - s_xdigits) / 2);
      }

    inline bool do_accumulate_digit(std::int64_t& value, std::int64_t limit, std::uint8_t base, std::uint8_t dvalue) noexcept
      {
        auto sbtm = limit >> 63;
        if(sbtm ? (value < (limit + dvalue) / base) : (value > (limit - dvalue) / base)) {
          return false;
        }
        value *= base;
        value += dvalue ^ sbtm;
        value -= sbtm;
        return true;
      }

    inline void do_raise(double& value, std::uint8_t base, std::int64_t exp) noexcept
      {
        if(exp > 0) {
          value *= power_u64(base, +static_cast<std::uint64_t>(exp));
        }
        if(exp < 0) {
          value /= power_u64(base, -static_cast<std::uint64_t>(exp));
        }
      }

    }

Opt<G_integer> std_numeric_parse_integer(const G_string& text)
  {
    auto tpos = text.find_first_not_of(s_spaces);
    if(tpos == G_string::npos) {
      // `text` consists of only spaces. Fail.
      return rocket::nullopt;
    }
    bool rneg = false;  // is the number negative?
    std::size_t rbegin = 0;  // beginning of significant figures
    std::size_t rend = 0;  // end of significant figures
    std::uint8_t rbase = 10;  // the base of the integral and fractional parts.
    std::uint8_t pbase = 0;  // the base of the exponent.
    std::int64_t pexp = 0;  // `pbase`'d exponent
    // Get the sign of the number if any.
    switch(text[tpos]) {
    case '+':
      tpos++;
      rneg = false;
      break;
    case '-':
      tpos++;
      rneg = true;
      break;
    }
    switch(text[tpos]) {
    case '0':
      // Check for the base prefix.
      tpos++;
      switch(text[tpos]) {
      case 'b':
      case 'B':
        tpos++;
        rbase = 2;
        break;
      case 'x':
      case 'X':
        tpos++;
        rbase = 16;
        break;
      }
      // Fallthrough
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      // Do nothing.
      break;
    default:
      // Fail.
      return rocket::nullopt;
    }
    rbegin = tpos;
    rend = tpos;
    // Parse the integral part.
    for(;;) {
      auto dvalue = do_translate_digit(text[tpos]);
      if(dvalue >= rbase) {
        break;
      }
      tpos++;
      // Accept a digit.
      rend = tpos;
      // Is the next character a digit separator?
      if(text[tpos] == '`') {
        tpos++;
      }
    }
    // Check for the exponent part.
    switch(text[tpos]) {
    case 'e':
    case 'E':
      tpos++;
      pbase = 10;
      break;
    case 'p':
    case 'P':
      tpos++;
      pbase = 2;
      break;
    }
    if(pbase != 0) {
      // Get the sign of the exponent if any.
      bool pneg = false;
      switch(text[tpos]) {
      case '+':
        tpos++;
        pneg = false;
        break;
      case '-':
        tpos++;
        pneg = true;
        break;
      }
      // Parse the exponent as an integer. The value must fit in 24 bits.
      for(;;) {
        auto dvalue = do_translate_digit(text[tpos]);
        if(dvalue >= 10) {
          break;
        }
        tpos++;
        // Accept a digit.
        if(!do_accumulate_digit(pexp, pneg ? -0x800000 : +0x7FFFFF, 10, dvalue)) {
          return rocket::nullopt;
        }
        // Is the next character a digit separator?
        if(text[tpos] == '`') {
          tpos++;
        }
      }
    }
    // Only spaces are allowed to follow the number.
    if(text.find_first_not_of(s_spaces, tpos) != G_string::npos) {
      return rocket::nullopt;
    }
    // The literal is an `integer` if there is no decimal point.
    std::int64_t value = 0;
    // Accumulate digits from left to right.
    for(auto ri = rbegin; ri != rend; ++ri) {
      auto dvalue = do_translate_digit(text[ri]);
      if(dvalue >= rbase) {
        continue;
      }
      // Accept a digit.
      if(!do_accumulate_digit(value, rneg ? INT64_MIN : INT64_MAX, rbase, dvalue)) {
        return rocket::nullopt;
      }
    }
    // Negative exponents are not allowed, not even when the significant part is zero.
    if(pexp < 0) {
      return rocket::nullopt;
    }
    // Raise the result.
    if(value != 0) {
      for(auto i = pexp; i != 0; --i) {
        // Append a digit zero.
        if(!do_accumulate_digit(value, rneg ? INT64_MIN : INT64_MAX, pbase, 0)) {
          return rocket::nullopt;
        }
      }
    }
    // Return the integer.
    return value;
  }

Opt<G_real> std_numeric_parse_real(const G_string& text, const Opt<G_boolean>& saturating)
  {
    auto tpos = text.find_first_not_of(s_spaces);
    if(tpos == G_string::npos) {
      // `text` consists of only spaces. Fail.
      return rocket::nullopt;
    }
    bool rneg = false;  // is the number negative?
    std::size_t rbegin = 0;  // beginning of significant figures
    std::size_t rend = 0;  // end of significant figures
    std::uint8_t rbase = 10;  // the base of the integral and fractional parts.
    std::int64_t icnt = 0;  // number of integral digits (always non-negative)
    std::int64_t fcnt = 0;  // number of fractional digits (always non-negative)
    std::uint8_t pbase = 0;  // the base of the exponent.
    std::int64_t pexp = 0;  // `pbase`'d exponent
    // Get the sign of the number if any.
    switch(text[tpos]) {
    case '+':
      tpos++;
      rneg = false;
      break;
    case '-':
      tpos++;
      rneg = true;
      break;
    }
    switch(text[tpos]) {
    case '0':
      // Check for the base prefix.
      tpos++;
      switch(text[tpos]) {
      case 'b':
      case 'B':
        tpos++;
        rbase = 2;
        break;
      case 'x':
      case 'X':
        tpos++;
        rbase = 16;
        break;
      }
      // Fallthrough
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      // Do nothing.
      break;
    case 'i':
    case 'I':
      // Check for infinities.
      if(do_compare_lowercase(text, tpos + 1, "nfinity", 7) != 0) {
        return rocket::nullopt;
      }
      tpos += 8;
      // Only spaces are allowed to follow the number.
      if(text.find_first_not_of(s_spaces, tpos) != G_string::npos) {
        return rocket::nullopt;
      }
      // Return a signed infinity.
      return std::copysign(INFINITY, -rneg);
    case 'n':
    case 'N':
      // Check for NaNs.
      if(do_compare_lowercase(text, tpos + 1, "an", 2) != 0) {
        return rocket::nullopt;
      }
      tpos += 3;
      // Only spaces are allowed to follow the number.
      if(text.find_first_not_of(s_spaces, tpos) != G_string::npos) {
        return rocket::nullopt;
      }
      // Return a signed NaN.
      return std::copysign(NAN, -rneg);
    default:
      // Fail.
      return rocket::nullopt;
    }
    rbegin = tpos;
    rend = tpos;
    // Parse the integral part.
    for(;;) {
      auto dvalue = do_translate_digit(text[tpos]);
      if(dvalue >= rbase) {
        break;
      }
      tpos++;
      // Accept a digit.
      rend = tpos;
      icnt++;
      // Is the next character a digit separator?
      if(text[tpos] == '`') {
        tpos++;
      }
    }
    // Check for the fractional part.
    if(text[tpos] == '.') {
      tpos++;
      // Parse the fractional part.
      for(;;) {
        auto dvalue = do_translate_digit(text[tpos]);
        if(dvalue >= rbase) {
          break;
        }
        tpos++;
        // Accept a digit.
        rend = tpos;
        fcnt++;
        // Is the next character a digit separator?
        if(text[tpos] == '`') {
          tpos++;
        }
      }
    }
    // Check for the exponent part.
    switch(text[tpos]) {
    case 'e':
    case 'E':
      tpos++;
      pbase = 10;
      break;
    case 'p':
    case 'P':
      tpos++;
      pbase = 2;
      break;
    }
    if(pbase != 0) {
      // Get the sign of the exponent if any.
      bool pneg = false;
      switch(text[tpos]) {
      case '+':
        tpos++;
        pneg = false;
        break;
      case '-':
        tpos++;
        pneg = true;
        break;
      }
      // Parse the exponent as an integer. The value must fit in 24 bits.
      for(;;) {
        auto dvalue = do_translate_digit(text[tpos]);
        if(dvalue >= 10) {
          break;
        }
        tpos++;
        // Accept a digit.
        if(!do_accumulate_digit(pexp, pneg ? -0x800000 : +0x7FFFFF, 10, dvalue)) {
          return rocket::nullopt;
        }
        // Is the next character a digit separator?
        if(text[tpos] == '`') {
          tpos++;
        }
      }
    }
    // Only spaces are allowed to follow the number.
    if(text.find_first_not_of(s_spaces, tpos) != G_string::npos) {
      return rocket::nullopt;
    }
    // Convert the value.
    if(rbase == pbase) {
      // Contract floating operations to minimize rounding errors.
      pexp -= fcnt;
      icnt += fcnt;
      fcnt = 0;
    }
    // Digits are accumulated using a 64-bit integer with no fractional part.
    // Excess significant figures are discard if the integer would overflow.
    std::int64_t tvalue = 0;
    std::int64_t tcnt = icnt;
    // Accumulate digits from left to right.
    for(auto ri = rbegin; ri != rend; ++ri) {
      auto dvalue = do_translate_digit(text[ri]);
      if(dvalue >= rbase) {
        continue;
      }
      // Accept a digit.
      if(!do_accumulate_digit(tvalue, rneg ? INT64_MIN : INT64_MAX, rbase, dvalue)) {
        break;
      }
      // Nudge the decimal point to the right.
      tcnt--;
    }
    // Raise the result.
    double value;
    if(tvalue == 0) {
      value = std::copysign(0.0, -rneg);
    } else {
      value = static_cast<double>(tvalue);
      do_raise(value, rbase, tcnt);
      do_raise(value, pbase, pexp);
    }
    // Check for overflow or underflow.
    int fpcls = std::fpclassify(value);
    if((fpcls == FP_INFINITE) && !saturating) {
      // Make sure we return an infinity only when the string is an explicit one.
      return rocket::nullopt;
    }
    // N.B. The sign bit of a negative zero shall have been preserved.
    return value;
  }

void create_bindings_numeric(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.numeric.integer_max`
    //===================================================================
    result.insert_or_assign(rocket::sref("integer_max"),
      G_integer(
        // The maximum value of an `integer`.
        INT64_MAX
      ));
    //===================================================================
    // `std.numeric.integer_min`
    //===================================================================
    result.insert_or_assign(rocket::sref("integer_min"),
      G_integer(
        // The minimum value of an `integer`.
        INT64_MIN
      ));
    //===================================================================
    // `std.numeric.real_max`
    //===================================================================
    result.insert_or_assign(rocket::sref("real_max"),
      G_real(
        // The maximum finite value of a `real`.
        DBL_MAX
      ));
    //===================================================================
    // `std.numeric.real_min`
    //===================================================================
    result.insert_or_assign(rocket::sref("real_min"),
      G_real(
        // The minimum finite value of a `real`.
        -DBL_MAX
      ));
    //===================================================================
    // `std.numeric.real_epsilon`
    //===================================================================
    result.insert_or_assign(rocket::sref("real_epsilon"),
      G_real(
        // The minimum finite value of a `real` such that `1 + real_epsilon > 1`.
        DBL_EPSILON
      ));
    //===================================================================
    // `std.numeric.size_max`
    //===================================================================
    result.insert_or_assign(rocket::sref("size_max"),
      G_integer(
        // The maximum length of a `string` or `array`.
        PTRDIFF_MAX
      ));
    //===================================================================
    // `std.numeric.abs()`
    //===================================================================
    result.insert_or_assign(rocket::sref("abs"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.abs(value)`\n"
            "  \n"
            "  * Gets the absolute value of `value`, which may be an `integer`\n"
            "    or `real`. Negative `integer`s are negated, which might cause\n"
            "    an exception to be thrown due to overflow. Sign bits of `real`s\n"
            "    are removed, which works on infinities and NaNs and does not\n"
            "    result in exceptions.\n"
            "  \n"
            "  * Return the absolute value.\n"
            "  \n"
            "  * Throws an exception if `value` is the `integer` `-0x1p63`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.abs"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_abs(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_abs(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.sign()`
    //===================================================================
    result.insert_or_assign(rocket::sref("sign"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.sign(value)`\n"
            "  \n"
            "  * Propagates the sign bit of the number `value`, which may be an\n"
            "    `integer` or `real`, to all bits of an `integer`. Be advised\n"
            "    that `-0.0` is distinct from `0.0` despite the equality.\n"
            "  \n"
            "  * Returns `-1` if `value` is negative, or `0` otherwise.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.sign"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_sign(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_sign(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.is_finite()`
    //===================================================================
    result.insert_or_assign(rocket::sref("is_finite"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.is_finite(value)`\n"
            "  \n"
            "  * Checks whether `value` is a finite number. `value` may be an\n"
            "    `integer` or `real`. Be adviced that this functions returns\n"
            "    `true` for `integer`s for consistency; `integer`s do not\n"
            "    support infinities or NaNs.\n"
            "  \n"
            "  * Returns `true` if `value` is an `integer` or is a `real` that\n"
            "    is neither an infinity or a NaN, or `false` otherwise.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.is_finite"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_finite(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_finite(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.is_infinity()`
    //===================================================================
    result.insert_or_assign(rocket::sref("is_infinity"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.is_infinity(value)`\n"
            "  \n"
            "  * Checks whether `value` is an infinity. `value` may be an\n"
            "    `integer` or `real`. Be adviced that this functions returns\n"
            "    `false` for `integer`s for consistency; `integer`s do not\n"
            "    support infinities.\n"
            "  \n"
            "  * Returns `true` if `value` is a `real` that denotes an infinity;\n"
            "    or `false` otherwise.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.is_infinity"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_infinity(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_infinity(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.is_nan()`
    //===================================================================
    result.insert_or_assign(rocket::sref("is_nan"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.is_nan(value)`\n"
            "  \n"
            "  * Checks whether `value` is a NaN. `value` may be an `integer` or\n"
            "    `real`. Be adviced that this functions returns `false` for\n"
            "    `integer`s for consistency; `integer`s do not support NaNs.\n"
            "  \n"
            "  * Returns `true` if `value` is a `real` denoting a NaN, or\n"
            "    `false` otherwise.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.is_nan"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_nan(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_nan(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.clamp()`
    //===================================================================
    result.insert_or_assign(rocket::sref("clamp"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.clamp(value, lower, upper)`\n"
            "  \n"
            "  * Limits `value` between `lower` and `upper`.\n"
            "  \n"
            "  * Returns `lower` if `value < lower`, `upper` if `value > upper`,\n"
            "    or `value` otherwise, including when `value` is a NaN. The\n"
            "    returned value is of type `integer` if all arguments are of\n"
            "    type `integer`; otherwise it is of type `real`.\n"
            "  \n"
            "  * Throws an exception if `lower` is not less than or equal to\n"
            "    `upper`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.clamp"), args);
            // Parse arguments.
            G_integer ivalue;
            G_integer ilower;
            G_integer iupper;
            if(reader.start().g(ivalue).g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_clamp(ivalue, ilower, iupper) };
              return rocket::move(xref);
            }
            G_real rvalue;
            G_real flower;
            G_real fupper;
            if(reader.start().g(rvalue).g(flower).g(fupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_clamp(rvalue, flower, fupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.round()`
    //===================================================================
    result.insert_or_assign(rocket::sref("round"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.round(value)`\n"
            "  \n"
            "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
            "    nearest integer; halfway values are rounded away from zero. If\n"
            "    `value` is an `integer`, it is returned intact.\n"
            "  \n"
            "  * Returns the rounded value.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.round"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_round(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_round(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.floor()`
    //===================================================================
    result.insert_or_assign(rocket::sref("floor"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.floor(value)`\n"
            "  \n"
            "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
            "    nearest integer towards negative infinity. If `value` is an\n"
            "    `integer`, it is returned intact.\n"
            "  \n"
            "  * Returns the rounded value.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.floor"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_floor(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_floor(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.ceil()`
    //===================================================================
    result.insert_or_assign(rocket::sref("ceil"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.ceil(value)`\n"
            "  \n"
            "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
            "    nearest integer towards positive infinity. If `value` is an\n"
            "    `integer`, it is returned intact.\n"
            "  \n"
            "  * Returns the rounded value.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.ceil"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_ceil(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_ceil(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.trunc()`
    //===================================================================
    result.insert_or_assign(rocket::sref("trunc"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.trunc(value)`\n"
            "  \n"
            "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
            "    nearest integer towards zero. If `value` is an `integer`, it is\n"
            "    returned intact.\n"
            "  \n"
            "  * Returns the rounded value.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.trunc"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_trunc(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_trunc(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.iround()`
    //===================================================================
    result.insert_or_assign(rocket::sref("iround"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.iround(value)`\n"
            "  \n"
            "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
            "    nearest integer; halfway values are rounded away from zero. If\n"
            "    `value` is an `integer`, it is returned intact. If `value` is a\n"
            "    `real`, it is converted to an `integer`.\n"
            "  \n"
            "  * Returns the rounded value as an `integer`.\n"
            "  \n"
            "  * Throws an exception if the result cannot be represented as an\n"
            "    `integer`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.iround"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_iround(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_iround(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.ifloor()`
    //===================================================================
    result.insert_or_assign(rocket::sref("ifloor"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.ifloor(value)`\n"
            "  \n"
            "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
            "    nearest integer towards negative infinity. If `value` is an\n"
            "    `integer`, it is returned intact. If `value` is a `real`, it is\n"
            "    converted to an `integer`.\n"
            "  \n"
            "  * Returns the rounded value as an `integer`.\n"
            "  \n"
            "  * Throws an exception if the result cannot be represented as an\n"
            "    `integer`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.ifloor"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_ifloor(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_ifloor(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.iceil()`
    //===================================================================
    result.insert_or_assign(rocket::sref("iceil"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.iceil(value)`\n"
            "  \n"
            "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
            "    nearest integer towards positive infinity. If `value` is an\n"
            "    `integer`, it is returned intact. If `value` is a `real`, it is\n"
            "    converted to an `integer`.\n"
            "  \n"
            "  * Returns the rounded value as an `integer`.\n"
            "  \n"
            "  * Throws an exception if the result cannot be represented as an\n"
            "    `integer`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.iceil"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_iceil(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_iceil(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.itrunc()`
    //===================================================================
    result.insert_or_assign(rocket::sref("itrunc"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.itrunc(value)`\n"
            "  \n"
            "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
            "    nearest integer towards zero. If `value` is an `integer`, it is\n"
            "    returned intact. If `value` is a `real`, it is converted to an\n"
            "    `integer`.\n"
            "  \n"
            "  * Returns the rounded value as an `integer`.\n"
            "  \n"
            "  * Throws an exception if the result cannot be represented as an\n"
            "    `integer`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.itrunc"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_itrunc(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_itrunc(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.random()`
    //===================================================================
    result.insert_or_assign(rocket::sref("random"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.random([limit])`\n"
            "  \n"
            "  * Generates a random `real` value whose sign agrees with `limit`\n"
            "    and whose absolute value is less than `limit`. If `limit` is\n"
            "    absent, `1` is assumed.\n"
            "  \n"
            "  * Returns a random `real` value.\n"
            "  \n"
            "  * Throws an exception if `limit` is zero or non-finite.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.random"), args);
            // Parse arguments.
            Opt<G_real> limit;
            if(reader.start().g(limit).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(global, limit) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.sqrt()`
    //===================================================================
    result.insert_or_assign(rocket::sref("sqrt"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.sqrt(x)`\n"
            "  \n"
            "  * Calculates the square root of `x` which may be of either the\n"
            "    `integer` or the `real` type. The result is always exact.\n"
            "  \n"
            "  * Returns the square root of `x` as a `real`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.sqrt"), args);
            // Parse arguments.
            G_real x;
            if(reader.start().g(x).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_sqrt(x) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.fma()`
    //===================================================================
    result.insert_or_assign(rocket::sref("fma"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.fma(x, y, z)`\n"
            "  \n"
            "  * Performs fused multiply-add operation on `x`, `y` and `z`. This\n"
            "    functions calculates `x * y + z` without intermediate rounding\n"
            "    operations. The result is always exact.\n"
            "  \n"
            "  * Returns the value of `x * y + z` as a `real`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.fma"), args);
            // Parse arguments.
            G_real x;
            G_real y;
            G_real z;
            if(reader.start().g(x).g(y).g(z).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_fma(x, y, z) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.addm()`
    //===================================================================
    result.insert_or_assign(rocket::sref("addm"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.addm(x, y)`\n"
            "  \n"
            "  * Adds `y` to `x` using modular arithmetic. `x` and `y` must be\n"
            "    of the `integer` type. The result is reduced to be congruent to\n"
            "    the sum of `x` and `y` modulo `0x1p64` in infinite precision.\n"
            "    This function will not cause overflow exceptions to be thrown.\n"
            "  \n"
            "  * Returns the reduced sum of `x` and `y`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.addm"), args);
            // Parse arguments.
            G_integer x;
            G_integer y;
            if(reader.start().g(x).g(y).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_addm(x, y) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.subm()`
    //===================================================================
    result.insert_or_assign(rocket::sref("subm"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.subm(x, y)`\n"
            "  \n"
            "  * Subtracts `y` from `x` using modular arithmetic. `x` and `y`\n"
            "    must be of the `integer` type. The result is reduced to be\n"
            "    congruent to the difference of `x` and `y` modulo `0x1p64` in\n"
            "    infinite precision. This function will not cause overflow\n"
            "    exceptions to be thrown.\n"
            "  \n"
            "  * Returns the reduced difference of `x` and `y`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.subm"), args);
            // Parse arguments.
            G_integer x;
            G_integer y;
            if(reader.start().g(x).g(y).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subm(x, y) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.mulm()`
    //===================================================================
    result.insert_or_assign(rocket::sref("mulm"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.mulm(x, y)`\n"
            "  \n"
            "  * Multiplies `x` by `y` using modular arithmetic. `x` and `y`\n"
            "    must be of the `integer` type. The result is reduced to be\n"
            "    congruent to the product of `x` and `y` modulo `0x1p64` in\n"
            "    infinite precision. This function will not cause overflow\n"
            "    exceptions to be thrown.\n"
            "  \n"
            "  * Returns the reduced product of `x` and `y`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.mulm"), args);
            // Parse arguments.
            G_integer x;
            G_integer y;
            if(reader.start().g(x).g(y).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_mulm(x, y) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.adds()`
    //===================================================================
    result.insert_or_assign(rocket::sref("adds"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.adds(x, y)`\n"
            "  \n"
            "  * Adds `y` to `x` using saturating arithmetic. `x` and `y` may be\n"
            "    `integer` or `real` values. The result is limited within the\n"
            "    range of representable values of its type, hence will not cause\n"
            "    overflow exceptions to be thrown. When either argument is of\n"
            "    type `real` which supports infinities, this function is\n"
            "    equivalent to the built-in addition operator.\n"
            "  \n"
            "  * Returns the saturated sum of `x` and `y`.\n"
            "\n"
            "`std.numeric.adds(x, y, lower, upper)`\n"
            "  \n"
            "  * Adds `y` to `x` using saturating arithmetic. `x` and `y` may be\n"
            "    `integer` or `real` values. The result is limited between\n"
            "    `lower` and `upper`, hence will not cause overflow exceptions\n"
            "    to be thrown.\n"
            "  \n"
            "  * Returns the saturated sum of `x` and `y`. The result is of type\n"
            "    `integer` if all arguments are of type `integer`; otherwise it\n"
            "    is of type `real`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.adds"), args);
            Argument_Reader::State istate, fstate;
            // Parse arguments.
            G_integer ix;
            G_integer iy;
            if(reader.start().g(ix).g(iy).save(istate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_adds(ix, iy) };
              return rocket::move(xref);
            }
            G_real fx;
            G_real fy;
            if(reader.start().g(fx).g(fy).save(fstate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_adds(fx, fy) };
              return rocket::move(xref);
            }
            G_integer ilower;
            G_integer iupper;
            if(reader.load(istate).g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_adds(ix, iy, ilower, iupper) };
              return rocket::move(xref);
            }
            G_real flower;
            G_real fupper;
            if(reader.load(fstate).g(flower).g(fupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_adds(fx, fy, flower, fupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.subs()`
    //===================================================================
    result.insert_or_assign(rocket::sref("subs"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.subs(x, y)`\n"
            "  \n"
            "  * Subtracts `y` from `x` using saturating arithmetic. `x` and `y`\n"
            "    may be `integer` or `real` values. The result is limited within\n"
            "    the range of representable values of its type, hence will not\n"
            "    cause overflow exceptions to be thrown. When either argument is\n"
            "    of type `real` which supports infinities, this function is\n"
            "    equivalent to the built-in subtraction operator.\n"
            "  \n"
            "  * Returns the saturated difference of `x` and `y`.\n"
            "\n"
            "`std.numeric.subs(x, y, lower, upper)`\n"
            "  \n"
            "  * Subtracts `y` from `x` using saturating arithmetic. `x` and `y`\n"
            "    may be `integer` or `real` values. The result is limited\n"
            "    between `lower` and `upper`, hence will not cause overflow\n"
            "    exceptions to be thrown.\n"
            "  \n"
            "  * Returns the saturated difference of `x` and `y`. The result is\n"
            "    of type `integer` if all arguments are of type `integer`;\n"
            "    otherwise it is of type `real`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.subs"), args);
            Argument_Reader::State istate, fstate;
            // Parse arguments.
            G_integer ix;
            G_integer iy;
            if(reader.start().g(ix).g(iy).save(istate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subs(ix, iy) };
              return rocket::move(xref);
            }
            G_real fx;
            G_real fy;
            if(reader.start().g(fx).g(fy).save(fstate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subs(fx, fy) };
              return rocket::move(xref);
            }
            G_integer ilower;
            G_integer iupper;
            if(reader.load(istate).g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subs(ix, iy, ilower, iupper) };
              return rocket::move(xref);
            }
            G_real flower;
            G_real fupper;
            if(reader.load(fstate).g(flower).g(fupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subs(fx, fy, flower, fupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.muls()`
    //===================================================================
    result.insert_or_assign(rocket::sref("muls"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.muls(x, y)`\n"
            "  \n"
            "  * Multiplies `x` by `y` using saturating arithmetic. `x` and `y`\n"
            "    may be `integer` or `real` values. The result is limited within\n"
            "    the range of representable values of its type, hence will not\n"
            "    cause overflow exceptions to be thrown. When either argument is\n"
            "    of type `real` which supports infinities, this function is\n"
            "    equivalent to the built-in multiplication operator.\n"
            "  \n"
            "  * Returns the saturated product of `x` and `y`.\n"
            "\n"
            "`std.numeric.muls(x, y, lower, upper)`\n"
            "  \n"
            "  * Multiplies `x` by `y` using saturating arithmetic. `x` and `y`\n"
            "    may be `integer` or `real` values. The result is limited\n"
            "    between `lower` and `upper`, hence will not cause overflow\n"
            "    exceptions to be thrown.\n"
            "  \n"
            "  * Returns the saturated product of `x` and `y`. The result is of\n"
            "    type `integer` if all arguments are of type `integer`;\n"
            "    otherwise it is of type `real`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.muls"), args);
            Argument_Reader::State istate, fstate;
            // Parse arguments.
            G_integer ix;
            G_integer iy;
            if(reader.start().g(ix).g(iy).save(istate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_muls(ix, iy) };
              return rocket::move(xref);
            }
            G_real fx;
            G_real fy;
            if(reader.start().g(fx).g(fy).save(fstate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_muls(fx, fy) };
              return rocket::move(xref);
            }
            G_integer ilower;
            G_integer iupper;
            if(reader.load(istate).g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_muls(ix, iy, ilower, iupper) };
              return rocket::move(xref);
            }
            G_real flower;
            G_real fupper;
            if(reader.load(fstate).g(flower).g(fupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_muls(fx, fy, flower, fupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.format()`
    //===================================================================
    result.insert_or_assign(rocket::sref("format"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.format(value, [base], [exp_base])`\n"
            "  \n"
            "  * Converts an `integer` or `real` to a `string` in `base`. This\n"
            "    function writes as many digits as possible to ensure precision.\n"
            "    No plus sign appears in the result. If `base` is not specified,\n"
            "    `10` is assumed. When `exp_base` is specified, if `value` is of\n"
            "    type `integer`, the significant figure part is kept as short as\n"
            "    possible without a decimal point, followed by an exponent part;\n"
            "    otherwise (when `value` is of type `real`), it is written in\n"
            "    scientific notation which always contains a decimal point.\n"
            "  \n"
            "  * Returns a `string` converted from `value`.\n"
            "  \n"
            "  * Throws an exception if `base` is neither `2` nor `10` nor `16`,\n"
            "    or if `exp_base` is neither `2` nor `10`, or if `base` is `16`\n"
            "    and `exp_base` is `10`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.format"), args);
            Argument_Reader::State istate, fstate;
            // Parse arguments.
            G_integer ivalue;
            Opt<G_integer> base;
            if(reader.start().g(ivalue).g(base).save(istate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_format(ivalue, base) };
              return rocket::move(xref);
            }
            G_real fvalue;
            if(reader.start().g(fvalue).g(base).save(fstate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_format(fvalue, base) };
              return rocket::move(xref);
            }
            G_integer exp_base;
            if(reader.load(istate).g(exp_base).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_format(ivalue, base, exp_base) };
              return rocket::move(xref);
            }
            if(reader.load(fstate).g(exp_base).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_format(fvalue, base, exp_base) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.parse_integer()`
    //===================================================================
    result.insert_or_assign(rocket::sref("parse_integer"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.parse_integer(text)`\n"
            "  \n"
            "  * Parses `text` for an `integer`. `text` shall be a `string`. All\n"
            "    leading and trailing blank characters are stripped from `text`.\n"
            "    If it becomes empty, this function fails; otherwise, it shall\n"
            "    match one of the following Perl regular expressions, ignoring\n"
            "    case of characters:\n"
            "    \n"
            "    * Binary (base-2):\n"
            "      `[+-]?0b([01]`?)+[ep][+]?([0-9]`?)+`\n"
            "    * Hexadecimal (base-16):\n"
            "      `[+-]?0x([0-9a-f]`?)+[ep][+]?([0-9]`?)+`\n"
            "    * Decimal (base-10):\n"
            "      `[+-]?([0-9]`?)+[ep][+]?([0-9]`?)+`\n"
            "    \n"
            "    If the string does not match any of the above, this function\n"
            "    fails. If the result is outside the range of representable\n"
            "    values of type `integer`, this function fails.\n"
            "  \n"
            "  * Returns the `integer` value converted from `text`. On failure,\n"
            "    `null` is returned.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.parse_integer"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto qres = std_numeric_parse_integer(text);
              if(!qres) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qres) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.numeric.parse_real()`
    //===================================================================
    result.insert_or_assign(rocket::sref("parse_real"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.numeric.parse_real(text, [saturating])`\n"
            "  \n"
            "  * Parses `text` for a `real`. `text` shall be a `string`. All\n"
            "    leading and trailing blank characters are stripped from `text`.\n"
            "    If it becomes empty, this function fails; otherwise, it shall\n"
            "    match any of the following Perl regular expressions, ignoring\n"
            "    case of characters:\n"
            "    \n"
            "    * Infinities:\n"
            "      `[+-]?infinity`\n"
            "    * NaNs:\n"
            "      `[+-]?nan`\n"
            "    * Binary (base-2):\n"
            "      `[+-]?0b([01]`?)+(\\.([01]`?))?[ep][-+]?([0-9]`?)+`\n"
            "    * Hexadecimal (base-16):\n"
            "      `[+-]?0x([0-9a-f]`?)+(\\.([0-9a-f]`?))?[ep][-+]?([0-9]`?)+`\n"
            "    * Decimal (base-10):\n"
            "      `[+-]?([0-9]`?)+(\\.([0-9]`?))?[ep][-+]?([0-9]`?)+`\n"
            "    \n"
            "    If the string does not match any of the above, this function\n"
            "    fails. If the absolute value of the result is too small to fit\n"
            "    in a `real`, a signed zero is returned. When the absolute value\n"
            "    is too large, if `saturating` is set to `true`, a signed\n"
            "    infinity is returned; otherwise this function fails.\n"
            "  \n"
            "  * Returns the `real` value converted from `text`. On failure,\n"
            "    `null` is returned.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.parse_real"), args);
            // Parse arguments.
            G_string text;
            Opt<G_boolean> saturating;
            if(reader.start().g(text).g(saturating).finish()) {
              // Call the binding function.
              auto qres = std_numeric_parse_real(text, saturating);
              if(!qres) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qres) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // End of `std.numeric`
    //===================================================================
  }

}  // namespace Asteria
