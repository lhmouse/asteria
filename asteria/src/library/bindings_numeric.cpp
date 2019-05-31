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
      if(fpcls == FP_ZERO) {
        ASTERIA_THROW_RUNTIME_ERROR("The `limit` for random numbers shall not be zero (got `", *limit, "`).");
      }
      if((fpcls == FP_INFINITE) || (fpcls == FP_NAN)) {
        ASTERIA_THROW_RUNTIME_ERROR("The `limit` for random numbers shall be finite (got `", *limit, "`).");
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

G_integer std_numeric_subs(const G_integer& x, const G_integer& y)
  {
    return do_saturing_sub(x, y);
  }

G_real std_numeric_subs(const G_real& x, const G_real& y)
  {
    return do_saturing_sub(x, y);
  }

G_integer std_numeric_muls(const G_integer& x, const G_integer& y)
  {
    return do_saturing_mul(x, y);
  }

G_real std_numeric_muls(const G_real& x, const G_real& y)
  {
    return do_saturing_mul(x, y);
  }

    namespace {

    constexpr char s_xdigits[] = "00112233445566778899aAbBcCdDeEfF";
    constexpr char s_spaces[] = " \f\n\r\t\v";

    void do_prepend(char*& bp, const char* s)
      {
        auto n = std::strlen(s);
        bp -= n;
        std::memcpy(bp, s, n);
      }

    void do_format_significand_integer(G_string& text, const G_integer& value, std::uint8_t rbase)
      {
        auto reg = value;
        auto sbtm = reg >> 63;
        // Write characters backwards.
        std::array<char, 72> temp;
        auto bp = temp.end();
        while(reg != 0) {
          // Shift a digit out.
          auto off = reg % rbase;
          reg /= rbase;
          // Get the absolute value of this digit.
          off ^= sbtm;
          off -= sbtm;
          // Locate the digit in uppercase.
          off *= 2;
          off += 1;
          // Write this digit.
          *--bp = s_xdigits[off];
        }
        // Ensure there is at least a zero digit.
        if(bp == temp.end()) {
          *--bp = '0';
        }
        // Prepend the base prefix.
        switch(rbase) {
        case  2:
          do_prepend(bp, "0b");
          break;
        case 16:
          do_prepend(bp, "0x");
          break;
        case 10:
          break;
        default:
          ROCKET_ASSERT(false);
        }
        // If the number is negative, prepend a minus sign.
        if(sbtm) {
          *--bp = '-';
        }
        // Append the result to `text` without clobbering existent contents.
        text.append(bp, temp.end());
      }

    void do_format_exponent(G_string& text, std::int32_t value, std::uint8_t pbase)
      {
        auto reg = value;
        auto sbtm = reg >> 31;
        // Write characters backwards.
        std::array<char, 16> temp;
        auto bp = temp.end();
        while(reg != 0) {
          // Shift a digit out.
          auto off = reg % 10;
          reg /= 10;
          // Get the absolute value of this digit.
          off ^= sbtm;
          off -= sbtm;
          // Locate the digit in uppercase.
          off *= 2;
          off += 1;
          // Write this digit.
          *--bp = s_xdigits[off];
        }
        // Ensure there are least two digits.
        while(temp.end() - bp < 2) {
          *--bp = '0';
        }
        // Prepend a plus or minus sign.
        if(sbtm) {
          *--bp = '-';
        } else {
          *--bp = '+';
        }
        // Prepend the base prefix.
        switch(pbase) {
        case  2:
          *--bp = 'p';
          break;
        case 10:
          *--bp = 'e';
          break;
        default:
          ROCKET_ASSERT(false);
        }
        // Append the result to `text` without clobbering existent contents.
        text.append(bp, temp.end());
      }

    G_string do_format_integer(const G_integer& value, std::uint8_t rbase)
      {
        // No exponent appears.
        G_string text;
        do_format_significand_integer(text, value, rbase);
        return text;
      }

    G_string do_format_integer_with_exponent(const G_integer& value, std::uint8_t rbase, std::uint8_t pbase)
      {
        auto reg = value;
        // Remove as many trailing zeroes as possible.
        int exp = 0;
        for(;;) {
          auto next = reg / pbase;
          if(next * pbase != reg) {
            break;
          }
          reg = next;
          exp++;
        }
        // Write the significand followed by the exponent.
        G_string text;
        do_format_significand_integer(text, reg, rbase);
        do_format_exponent(text, exp, pbase);
        return text;
      }

    }

G_string std_numeric_format(const G_integer& value, const Opt<G_integer>& base, const Opt<G_integer>& ebase)
  {
    switch(base.value_or(10)) {
    case  2:
      {
        if(!ebase) {
          return do_format_integer(value,  2);
        }
        if(*ebase ==  2) {
          return do_format_integer_with_exponent(value,  2,  2);
        }
        ASTERIA_THROW_RUNTIME_ERROR("The base of the exponent of a number in binary must be `2` (got `", *ebase, "`).");
      }
    case 16:
      {
        if(!ebase) {
          return do_format_integer(value, 16);
        }
        if(*ebase ==  2) {
          return do_format_integer_with_exponent(value, 16,  2);
        }
        ASTERIA_THROW_RUNTIME_ERROR("The base of the exponent of a number in hexadecimal must be `2` (got `", *ebase, "`).");
      }
    case 10:
      {
        if(!ebase) {
          return do_format_integer(value, 10);
        }
        if(*ebase ==  2) {
          return do_format_integer_with_exponent(value, 10,  2);
        }
        if(*ebase == 10) {
          return do_format_integer_with_exponent(value, 10, 10);
        }
        ASTERIA_THROW_RUNTIME_ERROR("The base of the exponent of a number in decimal must be either `2` or `10` (got `", *ebase, "`).");
      }
    default:
      ASTERIA_THROW_RUNTIME_ERROR("The base of a number must be either `2` or `10` or `16` (got `", *base, "`).");
    }
  }

    namespace {

    bool do_preformat_real(G_string& text, const G_real& value, std::uint8_t rbase)
      {
        auto sbtm = std::signbit(value) ? std::intptr_t(-1) : 0;
        // Return early in case of non-finite values.
        int fpcls = std::fpclassify(value);
        if(fpcls == FP_INFINITE) {
          text = rocket::sref("-infinity" + 1 + sbtm);
          return false;
        }
        if(fpcls == FP_NAN) {
          text = rocket::sref("-nan" + 1 + sbtm);
          return false;
        }
        // Although positive and negative zeroes are special, we don't treat them specially.
        // If the number is negative, prepend a minus sign.
        if(sbtm) {
          text.push_back('-');
        }
        // Prepend the base prefix.
        switch(rbase) {
        case  2:
          text.append("0b");
          break;
        case 16:
          text.append("0x");
          break;
        case 10:
          break;
        default:
          ROCKET_ASSERT(false);
        }
        // Digits are ready to be appended to `text`.
        return true;
      }

    struct S_real_parts
      {
        double reg;
        int exp;
      };

    constexpr int s_exp2p1[] =
      {
        2, 4, 8, 16, 32, 64, 128, 256
      };

    S_real_parts do_decompose_real_exact(const G_real& value, std::uint8_t pbase)
      {
        // Break the number down into the significand and exponent parts.
        // The result has to be exact.
        S_real_parts p;
        p.reg = std::frexp(std::fabs(value), std::addressof(p.exp));
        // Normalize the number.
        switch(pbase) {
        case  2:
          p.reg *= 2;
          p.exp -= 1;
          break;
        case 16:
          p.exp -= 1;
          p.reg *= s_exp2p1[p.exp & 3];
          p.exp >>= 2;
          break;
        default:
          ROCKET_ASSERT(false);
        }
        return p;
      }

    void do_extract_digit_real_exact(G_string& text, S_real_parts& p, std::uint8_t rbase)
      {
        // Shift a digit out.
        auto off = static_cast<int>(p.reg);
        p.reg -= off;
        p.reg *= rbase;
        p.exp--;
        // Locate the digit in uppercase.
        off *= 2;
        off += 1;
        // Write this digit.
        text.push_back(s_xdigits[off]);
      }

    G_string do_format_real_exact(const G_real& value, std::uint8_t rbase)
      {
        G_string text;
        if(!do_preformat_real(text, value, rbase)) {
          return text;
        }
        // Rewrite the value in scientific notation.
        auto p = do_decompose_real_exact(value, rbase);
        // If the value is less than one, write it after a `0.` prefix and prepend zeroes as needed.
        if(p.exp <= -1) {
          text.append("0.");
          text.append(static_cast<std::size_t>(-1 - p.exp), '0');
        }
        // Write all significant figures.
        while(p.reg != 0) {
          do_extract_digit_real_exact(text, p, rbase);
          // If it was the last one of the integral part, append a decimal point.
          if(p.exp == -1) {
            text.push_back('.');
          }
        }
        // Fill zeroes until the end of the integral part has been reached.
        if(p.exp > -1) {
          text.append(static_cast<std::size_t>(p.exp - -1), '0');
          text.push_back('.');
        }
        // The string will always contain a decimal point.
        // Append a zero digit if there is no fractional part.
        if(text.back() == '.') {
          text.push_back('0');
        }
        return text;
      }

    G_string do_format_real_exact_scientific(const G_real& value, std::uint8_t rbase, std::uint8_t pbase)
      {
        G_string text;
        if(!do_preformat_real(text, value, rbase)) {
          return text;
        }
        // Rewrite the value in scientific notation.
        auto p = do_decompose_real_exact(value, pbase);
        auto exp = p.exp;
        // Write the first significant figure that precedes the decimal point.
        do_extract_digit_real_exact(text, p, rbase);
        // Write the decimal point.
        text.push_back('.');
        // Write all significant figures that follow the decimal point.
        while(p.reg != 0) {
          do_extract_digit_real_exact(text, p, rbase);
        }
        // The string will always contain a decimal point.
        // Append a zero digit if there is no fractional part.
        if(text.back() == '.') {
          text.push_back('0');
        }
        // Write the exponent.
        do_format_exponent(text, exp, pbase);
        return text;
      }

    constexpr double s_decimals[]
      {
               0,         0,  3.0e-324,  4.0e-324,  5.0e-324,  6.0e-324,  7.0e-324,  8.0e-324,  9.0e-324,
        1.0e-323,  2.0e-323,  3.0e-323,  4.0e-323,  5.0e-323,  6.0e-323,  7.0e-323,  8.0e-323,  9.0e-323,
        1.0e-322,  2.0e-322,  3.0e-322,  4.0e-322,  5.0e-322,  6.0e-322,  7.0e-322,  8.0e-322,  9.0e-322,
        1.0e-321,  2.0e-321,  3.0e-321,  4.0e-321,  5.0e-321,  6.0e-321,  7.0e-321,  8.0e-321,  9.0e-321,
        1.0e-320,  2.0e-320,  3.0e-320,  4.0e-320,  5.0e-320,  6.0e-320,  7.0e-320,  8.0e-320,  9.0e-320,
        1.0e-319,  2.0e-319,  3.0e-319,  4.0e-319,  5.0e-319,  6.0e-319,  7.0e-319,  8.0e-319,  9.0e-319,
        1.0e-318,  2.0e-318,  3.0e-318,  4.0e-318,  5.0e-318,  6.0e-318,  7.0e-318,  8.0e-318,  9.0e-318,
        1.0e-317,  2.0e-317,  3.0e-317,  4.0e-317,  5.0e-317,  6.0e-317,  7.0e-317,  8.0e-317,  9.0e-317,
        1.0e-316,  2.0e-316,  3.0e-316,  4.0e-316,  5.0e-316,  6.0e-316,  7.0e-316,  8.0e-316,  9.0e-316,
        1.0e-315,  2.0e-315,  3.0e-315,  4.0e-315,  5.0e-315,  6.0e-315,  7.0e-315,  8.0e-315,  9.0e-315,
        1.0e-314,  2.0e-314,  3.0e-314,  4.0e-314,  5.0e-314,  6.0e-314,  7.0e-314,  8.0e-314,  9.0e-314,
        1.0e-313,  2.0e-313,  3.0e-313,  4.0e-313,  5.0e-313,  6.0e-313,  7.0e-313,  8.0e-313,  9.0e-313,
        1.0e-312,  2.0e-312,  3.0e-312,  4.0e-312,  5.0e-312,  6.0e-312,  7.0e-312,  8.0e-312,  9.0e-312,
        1.0e-311,  2.0e-311,  3.0e-311,  4.0e-311,  5.0e-311,  6.0e-311,  7.0e-311,  8.0e-311,  9.0e-311,
        1.0e-310,  2.0e-310,  3.0e-310,  4.0e-310,  5.0e-310,  6.0e-310,  7.0e-310,  8.0e-310,  9.0e-310,
        1.0e-309,  2.0e-309,  3.0e-309,  4.0e-309,  5.0e-309,  6.0e-309,  7.0e-309,  8.0e-309,  9.0e-309,
        1.0e-308,  2.0e-308,  3.0e-308,  4.0e-308,  5.0e-308,  6.0e-308,  7.0e-308,  8.0e-308,  9.0e-308,
        1.0e-307,  2.0e-307,  3.0e-307,  4.0e-307,  5.0e-307,  6.0e-307,  7.0e-307,  8.0e-307,  9.0e-307,
        1.0e-306,  2.0e-306,  3.0e-306,  4.0e-306,  5.0e-306,  6.0e-306,  7.0e-306,  8.0e-306,  9.0e-306,
        1.0e-305,  2.0e-305,  3.0e-305,  4.0e-305,  5.0e-305,  6.0e-305,  7.0e-305,  8.0e-305,  9.0e-305,
        1.0e-304,  2.0e-304,  3.0e-304,  4.0e-304,  5.0e-304,  6.0e-304,  7.0e-304,  8.0e-304,  9.0e-304,
        1.0e-303,  2.0e-303,  3.0e-303,  4.0e-303,  5.0e-303,  6.0e-303,  7.0e-303,  8.0e-303,  9.0e-303,
        1.0e-302,  2.0e-302,  3.0e-302,  4.0e-302,  5.0e-302,  6.0e-302,  7.0e-302,  8.0e-302,  9.0e-302,
        1.0e-301,  2.0e-301,  3.0e-301,  4.0e-301,  5.0e-301,  6.0e-301,  7.0e-301,  8.0e-301,  9.0e-301,
        1.0e-300,  2.0e-300,  3.0e-300,  4.0e-300,  5.0e-300,  6.0e-300,  7.0e-300,  8.0e-300,  9.0e-300,
        1.0e-299,  2.0e-299,  3.0e-299,  4.0e-299,  5.0e-299,  6.0e-299,  7.0e-299,  8.0e-299,  9.0e-299,
        1.0e-298,  2.0e-298,  3.0e-298,  4.0e-298,  5.0e-298,  6.0e-298,  7.0e-298,  8.0e-298,  9.0e-298,
        1.0e-297,  2.0e-297,  3.0e-297,  4.0e-297,  5.0e-297,  6.0e-297,  7.0e-297,  8.0e-297,  9.0e-297,
        1.0e-296,  2.0e-296,  3.0e-296,  4.0e-296,  5.0e-296,  6.0e-296,  7.0e-296,  8.0e-296,  9.0e-296,
        1.0e-295,  2.0e-295,  3.0e-295,  4.0e-295,  5.0e-295,  6.0e-295,  7.0e-295,  8.0e-295,  9.0e-295,
        1.0e-294,  2.0e-294,  3.0e-294,  4.0e-294,  5.0e-294,  6.0e-294,  7.0e-294,  8.0e-294,  9.0e-294,
        1.0e-293,  2.0e-293,  3.0e-293,  4.0e-293,  5.0e-293,  6.0e-293,  7.0e-293,  8.0e-293,  9.0e-293,
        1.0e-292,  2.0e-292,  3.0e-292,  4.0e-292,  5.0e-292,  6.0e-292,  7.0e-292,  8.0e-292,  9.0e-292,
        1.0e-291,  2.0e-291,  3.0e-291,  4.0e-291,  5.0e-291,  6.0e-291,  7.0e-291,  8.0e-291,  9.0e-291,
        1.0e-290,  2.0e-290,  3.0e-290,  4.0e-290,  5.0e-290,  6.0e-290,  7.0e-290,  8.0e-290,  9.0e-290,
        1.0e-289,  2.0e-289,  3.0e-289,  4.0e-289,  5.0e-289,  6.0e-289,  7.0e-289,  8.0e-289,  9.0e-289,
        1.0e-288,  2.0e-288,  3.0e-288,  4.0e-288,  5.0e-288,  6.0e-288,  7.0e-288,  8.0e-288,  9.0e-288,
        1.0e-287,  2.0e-287,  3.0e-287,  4.0e-287,  5.0e-287,  6.0e-287,  7.0e-287,  8.0e-287,  9.0e-287,
        1.0e-286,  2.0e-286,  3.0e-286,  4.0e-286,  5.0e-286,  6.0e-286,  7.0e-286,  8.0e-286,  9.0e-286,
        1.0e-285,  2.0e-285,  3.0e-285,  4.0e-285,  5.0e-285,  6.0e-285,  7.0e-285,  8.0e-285,  9.0e-285,
        1.0e-284,  2.0e-284,  3.0e-284,  4.0e-284,  5.0e-284,  6.0e-284,  7.0e-284,  8.0e-284,  9.0e-284,
        1.0e-283,  2.0e-283,  3.0e-283,  4.0e-283,  5.0e-283,  6.0e-283,  7.0e-283,  8.0e-283,  9.0e-283,
        1.0e-282,  2.0e-282,  3.0e-282,  4.0e-282,  5.0e-282,  6.0e-282,  7.0e-282,  8.0e-282,  9.0e-282,
        1.0e-281,  2.0e-281,  3.0e-281,  4.0e-281,  5.0e-281,  6.0e-281,  7.0e-281,  8.0e-281,  9.0e-281,
        1.0e-280,  2.0e-280,  3.0e-280,  4.0e-280,  5.0e-280,  6.0e-280,  7.0e-280,  8.0e-280,  9.0e-280,
        1.0e-279,  2.0e-279,  3.0e-279,  4.0e-279,  5.0e-279,  6.0e-279,  7.0e-279,  8.0e-279,  9.0e-279,
        1.0e-278,  2.0e-278,  3.0e-278,  4.0e-278,  5.0e-278,  6.0e-278,  7.0e-278,  8.0e-278,  9.0e-278,
        1.0e-277,  2.0e-277,  3.0e-277,  4.0e-277,  5.0e-277,  6.0e-277,  7.0e-277,  8.0e-277,  9.0e-277,
        1.0e-276,  2.0e-276,  3.0e-276,  4.0e-276,  5.0e-276,  6.0e-276,  7.0e-276,  8.0e-276,  9.0e-276,
        1.0e-275,  2.0e-275,  3.0e-275,  4.0e-275,  5.0e-275,  6.0e-275,  7.0e-275,  8.0e-275,  9.0e-275,
        1.0e-274,  2.0e-274,  3.0e-274,  4.0e-274,  5.0e-274,  6.0e-274,  7.0e-274,  8.0e-274,  9.0e-274,
        1.0e-273,  2.0e-273,  3.0e-273,  4.0e-273,  5.0e-273,  6.0e-273,  7.0e-273,  8.0e-273,  9.0e-273,
        1.0e-272,  2.0e-272,  3.0e-272,  4.0e-272,  5.0e-272,  6.0e-272,  7.0e-272,  8.0e-272,  9.0e-272,
        1.0e-271,  2.0e-271,  3.0e-271,  4.0e-271,  5.0e-271,  6.0e-271,  7.0e-271,  8.0e-271,  9.0e-271,
        1.0e-270,  2.0e-270,  3.0e-270,  4.0e-270,  5.0e-270,  6.0e-270,  7.0e-270,  8.0e-270,  9.0e-270,
        1.0e-269,  2.0e-269,  3.0e-269,  4.0e-269,  5.0e-269,  6.0e-269,  7.0e-269,  8.0e-269,  9.0e-269,
        1.0e-268,  2.0e-268,  3.0e-268,  4.0e-268,  5.0e-268,  6.0e-268,  7.0e-268,  8.0e-268,  9.0e-268,
        1.0e-267,  2.0e-267,  3.0e-267,  4.0e-267,  5.0e-267,  6.0e-267,  7.0e-267,  8.0e-267,  9.0e-267,
        1.0e-266,  2.0e-266,  3.0e-266,  4.0e-266,  5.0e-266,  6.0e-266,  7.0e-266,  8.0e-266,  9.0e-266,
        1.0e-265,  2.0e-265,  3.0e-265,  4.0e-265,  5.0e-265,  6.0e-265,  7.0e-265,  8.0e-265,  9.0e-265,
        1.0e-264,  2.0e-264,  3.0e-264,  4.0e-264,  5.0e-264,  6.0e-264,  7.0e-264,  8.0e-264,  9.0e-264,
        1.0e-263,  2.0e-263,  3.0e-263,  4.0e-263,  5.0e-263,  6.0e-263,  7.0e-263,  8.0e-263,  9.0e-263,
        1.0e-262,  2.0e-262,  3.0e-262,  4.0e-262,  5.0e-262,  6.0e-262,  7.0e-262,  8.0e-262,  9.0e-262,
        1.0e-261,  2.0e-261,  3.0e-261,  4.0e-261,  5.0e-261,  6.0e-261,  7.0e-261,  8.0e-261,  9.0e-261,
        1.0e-260,  2.0e-260,  3.0e-260,  4.0e-260,  5.0e-260,  6.0e-260,  7.0e-260,  8.0e-260,  9.0e-260,
        1.0e-259,  2.0e-259,  3.0e-259,  4.0e-259,  5.0e-259,  6.0e-259,  7.0e-259,  8.0e-259,  9.0e-259,
        1.0e-258,  2.0e-258,  3.0e-258,  4.0e-258,  5.0e-258,  6.0e-258,  7.0e-258,  8.0e-258,  9.0e-258,
        1.0e-257,  2.0e-257,  3.0e-257,  4.0e-257,  5.0e-257,  6.0e-257,  7.0e-257,  8.0e-257,  9.0e-257,
        1.0e-256,  2.0e-256,  3.0e-256,  4.0e-256,  5.0e-256,  6.0e-256,  7.0e-256,  8.0e-256,  9.0e-256,
        1.0e-255,  2.0e-255,  3.0e-255,  4.0e-255,  5.0e-255,  6.0e-255,  7.0e-255,  8.0e-255,  9.0e-255,
        1.0e-254,  2.0e-254,  3.0e-254,  4.0e-254,  5.0e-254,  6.0e-254,  7.0e-254,  8.0e-254,  9.0e-254,
        1.0e-253,  2.0e-253,  3.0e-253,  4.0e-253,  5.0e-253,  6.0e-253,  7.0e-253,  8.0e-253,  9.0e-253,
        1.0e-252,  2.0e-252,  3.0e-252,  4.0e-252,  5.0e-252,  6.0e-252,  7.0e-252,  8.0e-252,  9.0e-252,
        1.0e-251,  2.0e-251,  3.0e-251,  4.0e-251,  5.0e-251,  6.0e-251,  7.0e-251,  8.0e-251,  9.0e-251,
        1.0e-250,  2.0e-250,  3.0e-250,  4.0e-250,  5.0e-250,  6.0e-250,  7.0e-250,  8.0e-250,  9.0e-250,
        1.0e-249,  2.0e-249,  3.0e-249,  4.0e-249,  5.0e-249,  6.0e-249,  7.0e-249,  8.0e-249,  9.0e-249,
        1.0e-248,  2.0e-248,  3.0e-248,  4.0e-248,  5.0e-248,  6.0e-248,  7.0e-248,  8.0e-248,  9.0e-248,
        1.0e-247,  2.0e-247,  3.0e-247,  4.0e-247,  5.0e-247,  6.0e-247,  7.0e-247,  8.0e-247,  9.0e-247,
        1.0e-246,  2.0e-246,  3.0e-246,  4.0e-246,  5.0e-246,  6.0e-246,  7.0e-246,  8.0e-246,  9.0e-246,
        1.0e-245,  2.0e-245,  3.0e-245,  4.0e-245,  5.0e-245,  6.0e-245,  7.0e-245,  8.0e-245,  9.0e-245,
        1.0e-244,  2.0e-244,  3.0e-244,  4.0e-244,  5.0e-244,  6.0e-244,  7.0e-244,  8.0e-244,  9.0e-244,
        1.0e-243,  2.0e-243,  3.0e-243,  4.0e-243,  5.0e-243,  6.0e-243,  7.0e-243,  8.0e-243,  9.0e-243,
        1.0e-242,  2.0e-242,  3.0e-242,  4.0e-242,  5.0e-242,  6.0e-242,  7.0e-242,  8.0e-242,  9.0e-242,
        1.0e-241,  2.0e-241,  3.0e-241,  4.0e-241,  5.0e-241,  6.0e-241,  7.0e-241,  8.0e-241,  9.0e-241,
        1.0e-240,  2.0e-240,  3.0e-240,  4.0e-240,  5.0e-240,  6.0e-240,  7.0e-240,  8.0e-240,  9.0e-240,
        1.0e-239,  2.0e-239,  3.0e-239,  4.0e-239,  5.0e-239,  6.0e-239,  7.0e-239,  8.0e-239,  9.0e-239,
        1.0e-238,  2.0e-238,  3.0e-238,  4.0e-238,  5.0e-238,  6.0e-238,  7.0e-238,  8.0e-238,  9.0e-238,
        1.0e-237,  2.0e-237,  3.0e-237,  4.0e-237,  5.0e-237,  6.0e-237,  7.0e-237,  8.0e-237,  9.0e-237,
        1.0e-236,  2.0e-236,  3.0e-236,  4.0e-236,  5.0e-236,  6.0e-236,  7.0e-236,  8.0e-236,  9.0e-236,
        1.0e-235,  2.0e-235,  3.0e-235,  4.0e-235,  5.0e-235,  6.0e-235,  7.0e-235,  8.0e-235,  9.0e-235,
        1.0e-234,  2.0e-234,  3.0e-234,  4.0e-234,  5.0e-234,  6.0e-234,  7.0e-234,  8.0e-234,  9.0e-234,
        1.0e-233,  2.0e-233,  3.0e-233,  4.0e-233,  5.0e-233,  6.0e-233,  7.0e-233,  8.0e-233,  9.0e-233,
        1.0e-232,  2.0e-232,  3.0e-232,  4.0e-232,  5.0e-232,  6.0e-232,  7.0e-232,  8.0e-232,  9.0e-232,
        1.0e-231,  2.0e-231,  3.0e-231,  4.0e-231,  5.0e-231,  6.0e-231,  7.0e-231,  8.0e-231,  9.0e-231,
        1.0e-230,  2.0e-230,  3.0e-230,  4.0e-230,  5.0e-230,  6.0e-230,  7.0e-230,  8.0e-230,  9.0e-230,
        1.0e-229,  2.0e-229,  3.0e-229,  4.0e-229,  5.0e-229,  6.0e-229,  7.0e-229,  8.0e-229,  9.0e-229,
        1.0e-228,  2.0e-228,  3.0e-228,  4.0e-228,  5.0e-228,  6.0e-228,  7.0e-228,  8.0e-228,  9.0e-228,
        1.0e-227,  2.0e-227,  3.0e-227,  4.0e-227,  5.0e-227,  6.0e-227,  7.0e-227,  8.0e-227,  9.0e-227,
        1.0e-226,  2.0e-226,  3.0e-226,  4.0e-226,  5.0e-226,  6.0e-226,  7.0e-226,  8.0e-226,  9.0e-226,
        1.0e-225,  2.0e-225,  3.0e-225,  4.0e-225,  5.0e-225,  6.0e-225,  7.0e-225,  8.0e-225,  9.0e-225,
        1.0e-224,  2.0e-224,  3.0e-224,  4.0e-224,  5.0e-224,  6.0e-224,  7.0e-224,  8.0e-224,  9.0e-224,
        1.0e-223,  2.0e-223,  3.0e-223,  4.0e-223,  5.0e-223,  6.0e-223,  7.0e-223,  8.0e-223,  9.0e-223,
        1.0e-222,  2.0e-222,  3.0e-222,  4.0e-222,  5.0e-222,  6.0e-222,  7.0e-222,  8.0e-222,  9.0e-222,
        1.0e-221,  2.0e-221,  3.0e-221,  4.0e-221,  5.0e-221,  6.0e-221,  7.0e-221,  8.0e-221,  9.0e-221,
        1.0e-220,  2.0e-220,  3.0e-220,  4.0e-220,  5.0e-220,  6.0e-220,  7.0e-220,  8.0e-220,  9.0e-220,
        1.0e-219,  2.0e-219,  3.0e-219,  4.0e-219,  5.0e-219,  6.0e-219,  7.0e-219,  8.0e-219,  9.0e-219,
        1.0e-218,  2.0e-218,  3.0e-218,  4.0e-218,  5.0e-218,  6.0e-218,  7.0e-218,  8.0e-218,  9.0e-218,
        1.0e-217,  2.0e-217,  3.0e-217,  4.0e-217,  5.0e-217,  6.0e-217,  7.0e-217,  8.0e-217,  9.0e-217,
        1.0e-216,  2.0e-216,  3.0e-216,  4.0e-216,  5.0e-216,  6.0e-216,  7.0e-216,  8.0e-216,  9.0e-216,
        1.0e-215,  2.0e-215,  3.0e-215,  4.0e-215,  5.0e-215,  6.0e-215,  7.0e-215,  8.0e-215,  9.0e-215,
        1.0e-214,  2.0e-214,  3.0e-214,  4.0e-214,  5.0e-214,  6.0e-214,  7.0e-214,  8.0e-214,  9.0e-214,
        1.0e-213,  2.0e-213,  3.0e-213,  4.0e-213,  5.0e-213,  6.0e-213,  7.0e-213,  8.0e-213,  9.0e-213,
        1.0e-212,  2.0e-212,  3.0e-212,  4.0e-212,  5.0e-212,  6.0e-212,  7.0e-212,  8.0e-212,  9.0e-212,
        1.0e-211,  2.0e-211,  3.0e-211,  4.0e-211,  5.0e-211,  6.0e-211,  7.0e-211,  8.0e-211,  9.0e-211,
        1.0e-210,  2.0e-210,  3.0e-210,  4.0e-210,  5.0e-210,  6.0e-210,  7.0e-210,  8.0e-210,  9.0e-210,
        1.0e-209,  2.0e-209,  3.0e-209,  4.0e-209,  5.0e-209,  6.0e-209,  7.0e-209,  8.0e-209,  9.0e-209,
        1.0e-208,  2.0e-208,  3.0e-208,  4.0e-208,  5.0e-208,  6.0e-208,  7.0e-208,  8.0e-208,  9.0e-208,
        1.0e-207,  2.0e-207,  3.0e-207,  4.0e-207,  5.0e-207,  6.0e-207,  7.0e-207,  8.0e-207,  9.0e-207,
        1.0e-206,  2.0e-206,  3.0e-206,  4.0e-206,  5.0e-206,  6.0e-206,  7.0e-206,  8.0e-206,  9.0e-206,
        1.0e-205,  2.0e-205,  3.0e-205,  4.0e-205,  5.0e-205,  6.0e-205,  7.0e-205,  8.0e-205,  9.0e-205,
        1.0e-204,  2.0e-204,  3.0e-204,  4.0e-204,  5.0e-204,  6.0e-204,  7.0e-204,  8.0e-204,  9.0e-204,
        1.0e-203,  2.0e-203,  3.0e-203,  4.0e-203,  5.0e-203,  6.0e-203,  7.0e-203,  8.0e-203,  9.0e-203,
        1.0e-202,  2.0e-202,  3.0e-202,  4.0e-202,  5.0e-202,  6.0e-202,  7.0e-202,  8.0e-202,  9.0e-202,
        1.0e-201,  2.0e-201,  3.0e-201,  4.0e-201,  5.0e-201,  6.0e-201,  7.0e-201,  8.0e-201,  9.0e-201,
        1.0e-200,  2.0e-200,  3.0e-200,  4.0e-200,  5.0e-200,  6.0e-200,  7.0e-200,  8.0e-200,  9.0e-200,
        1.0e-199,  2.0e-199,  3.0e-199,  4.0e-199,  5.0e-199,  6.0e-199,  7.0e-199,  8.0e-199,  9.0e-199,
        1.0e-198,  2.0e-198,  3.0e-198,  4.0e-198,  5.0e-198,  6.0e-198,  7.0e-198,  8.0e-198,  9.0e-198,
        1.0e-197,  2.0e-197,  3.0e-197,  4.0e-197,  5.0e-197,  6.0e-197,  7.0e-197,  8.0e-197,  9.0e-197,
        1.0e-196,  2.0e-196,  3.0e-196,  4.0e-196,  5.0e-196,  6.0e-196,  7.0e-196,  8.0e-196,  9.0e-196,
        1.0e-195,  2.0e-195,  3.0e-195,  4.0e-195,  5.0e-195,  6.0e-195,  7.0e-195,  8.0e-195,  9.0e-195,
        1.0e-194,  2.0e-194,  3.0e-194,  4.0e-194,  5.0e-194,  6.0e-194,  7.0e-194,  8.0e-194,  9.0e-194,
        1.0e-193,  2.0e-193,  3.0e-193,  4.0e-193,  5.0e-193,  6.0e-193,  7.0e-193,  8.0e-193,  9.0e-193,
        1.0e-192,  2.0e-192,  3.0e-192,  4.0e-192,  5.0e-192,  6.0e-192,  7.0e-192,  8.0e-192,  9.0e-192,
        1.0e-191,  2.0e-191,  3.0e-191,  4.0e-191,  5.0e-191,  6.0e-191,  7.0e-191,  8.0e-191,  9.0e-191,
        1.0e-190,  2.0e-190,  3.0e-190,  4.0e-190,  5.0e-190,  6.0e-190,  7.0e-190,  8.0e-190,  9.0e-190,
        1.0e-189,  2.0e-189,  3.0e-189,  4.0e-189,  5.0e-189,  6.0e-189,  7.0e-189,  8.0e-189,  9.0e-189,
        1.0e-188,  2.0e-188,  3.0e-188,  4.0e-188,  5.0e-188,  6.0e-188,  7.0e-188,  8.0e-188,  9.0e-188,
        1.0e-187,  2.0e-187,  3.0e-187,  4.0e-187,  5.0e-187,  6.0e-187,  7.0e-187,  8.0e-187,  9.0e-187,
        1.0e-186,  2.0e-186,  3.0e-186,  4.0e-186,  5.0e-186,  6.0e-186,  7.0e-186,  8.0e-186,  9.0e-186,
        1.0e-185,  2.0e-185,  3.0e-185,  4.0e-185,  5.0e-185,  6.0e-185,  7.0e-185,  8.0e-185,  9.0e-185,
        1.0e-184,  2.0e-184,  3.0e-184,  4.0e-184,  5.0e-184,  6.0e-184,  7.0e-184,  8.0e-184,  9.0e-184,
        1.0e-183,  2.0e-183,  3.0e-183,  4.0e-183,  5.0e-183,  6.0e-183,  7.0e-183,  8.0e-183,  9.0e-183,
        1.0e-182,  2.0e-182,  3.0e-182,  4.0e-182,  5.0e-182,  6.0e-182,  7.0e-182,  8.0e-182,  9.0e-182,
        1.0e-181,  2.0e-181,  3.0e-181,  4.0e-181,  5.0e-181,  6.0e-181,  7.0e-181,  8.0e-181,  9.0e-181,
        1.0e-180,  2.0e-180,  3.0e-180,  4.0e-180,  5.0e-180,  6.0e-180,  7.0e-180,  8.0e-180,  9.0e-180,
        1.0e-179,  2.0e-179,  3.0e-179,  4.0e-179,  5.0e-179,  6.0e-179,  7.0e-179,  8.0e-179,  9.0e-179,
        1.0e-178,  2.0e-178,  3.0e-178,  4.0e-178,  5.0e-178,  6.0e-178,  7.0e-178,  8.0e-178,  9.0e-178,
        1.0e-177,  2.0e-177,  3.0e-177,  4.0e-177,  5.0e-177,  6.0e-177,  7.0e-177,  8.0e-177,  9.0e-177,
        1.0e-176,  2.0e-176,  3.0e-176,  4.0e-176,  5.0e-176,  6.0e-176,  7.0e-176,  8.0e-176,  9.0e-176,
        1.0e-175,  2.0e-175,  3.0e-175,  4.0e-175,  5.0e-175,  6.0e-175,  7.0e-175,  8.0e-175,  9.0e-175,
        1.0e-174,  2.0e-174,  3.0e-174,  4.0e-174,  5.0e-174,  6.0e-174,  7.0e-174,  8.0e-174,  9.0e-174,
        1.0e-173,  2.0e-173,  3.0e-173,  4.0e-173,  5.0e-173,  6.0e-173,  7.0e-173,  8.0e-173,  9.0e-173,
        1.0e-172,  2.0e-172,  3.0e-172,  4.0e-172,  5.0e-172,  6.0e-172,  7.0e-172,  8.0e-172,  9.0e-172,
        1.0e-171,  2.0e-171,  3.0e-171,  4.0e-171,  5.0e-171,  6.0e-171,  7.0e-171,  8.0e-171,  9.0e-171,
        1.0e-170,  2.0e-170,  3.0e-170,  4.0e-170,  5.0e-170,  6.0e-170,  7.0e-170,  8.0e-170,  9.0e-170,
        1.0e-169,  2.0e-169,  3.0e-169,  4.0e-169,  5.0e-169,  6.0e-169,  7.0e-169,  8.0e-169,  9.0e-169,
        1.0e-168,  2.0e-168,  3.0e-168,  4.0e-168,  5.0e-168,  6.0e-168,  7.0e-168,  8.0e-168,  9.0e-168,
        1.0e-167,  2.0e-167,  3.0e-167,  4.0e-167,  5.0e-167,  6.0e-167,  7.0e-167,  8.0e-167,  9.0e-167,
        1.0e-166,  2.0e-166,  3.0e-166,  4.0e-166,  5.0e-166,  6.0e-166,  7.0e-166,  8.0e-166,  9.0e-166,
        1.0e-165,  2.0e-165,  3.0e-165,  4.0e-165,  5.0e-165,  6.0e-165,  7.0e-165,  8.0e-165,  9.0e-165,
        1.0e-164,  2.0e-164,  3.0e-164,  4.0e-164,  5.0e-164,  6.0e-164,  7.0e-164,  8.0e-164,  9.0e-164,
        1.0e-163,  2.0e-163,  3.0e-163,  4.0e-163,  5.0e-163,  6.0e-163,  7.0e-163,  8.0e-163,  9.0e-163,
        1.0e-162,  2.0e-162,  3.0e-162,  4.0e-162,  5.0e-162,  6.0e-162,  7.0e-162,  8.0e-162,  9.0e-162,
        1.0e-161,  2.0e-161,  3.0e-161,  4.0e-161,  5.0e-161,  6.0e-161,  7.0e-161,  8.0e-161,  9.0e-161,
        1.0e-160,  2.0e-160,  3.0e-160,  4.0e-160,  5.0e-160,  6.0e-160,  7.0e-160,  8.0e-160,  9.0e-160,
        1.0e-159,  2.0e-159,  3.0e-159,  4.0e-159,  5.0e-159,  6.0e-159,  7.0e-159,  8.0e-159,  9.0e-159,
        1.0e-158,  2.0e-158,  3.0e-158,  4.0e-158,  5.0e-158,  6.0e-158,  7.0e-158,  8.0e-158,  9.0e-158,
        1.0e-157,  2.0e-157,  3.0e-157,  4.0e-157,  5.0e-157,  6.0e-157,  7.0e-157,  8.0e-157,  9.0e-157,
        1.0e-156,  2.0e-156,  3.0e-156,  4.0e-156,  5.0e-156,  6.0e-156,  7.0e-156,  8.0e-156,  9.0e-156,
        1.0e-155,  2.0e-155,  3.0e-155,  4.0e-155,  5.0e-155,  6.0e-155,  7.0e-155,  8.0e-155,  9.0e-155,
        1.0e-154,  2.0e-154,  3.0e-154,  4.0e-154,  5.0e-154,  6.0e-154,  7.0e-154,  8.0e-154,  9.0e-154,
        1.0e-153,  2.0e-153,  3.0e-153,  4.0e-153,  5.0e-153,  6.0e-153,  7.0e-153,  8.0e-153,  9.0e-153,
        1.0e-152,  2.0e-152,  3.0e-152,  4.0e-152,  5.0e-152,  6.0e-152,  7.0e-152,  8.0e-152,  9.0e-152,
        1.0e-151,  2.0e-151,  3.0e-151,  4.0e-151,  5.0e-151,  6.0e-151,  7.0e-151,  8.0e-151,  9.0e-151,
        1.0e-150,  2.0e-150,  3.0e-150,  4.0e-150,  5.0e-150,  6.0e-150,  7.0e-150,  8.0e-150,  9.0e-150,
        1.0e-149,  2.0e-149,  3.0e-149,  4.0e-149,  5.0e-149,  6.0e-149,  7.0e-149,  8.0e-149,  9.0e-149,
        1.0e-148,  2.0e-148,  3.0e-148,  4.0e-148,  5.0e-148,  6.0e-148,  7.0e-148,  8.0e-148,  9.0e-148,
        1.0e-147,  2.0e-147,  3.0e-147,  4.0e-147,  5.0e-147,  6.0e-147,  7.0e-147,  8.0e-147,  9.0e-147,
        1.0e-146,  2.0e-146,  3.0e-146,  4.0e-146,  5.0e-146,  6.0e-146,  7.0e-146,  8.0e-146,  9.0e-146,
        1.0e-145,  2.0e-145,  3.0e-145,  4.0e-145,  5.0e-145,  6.0e-145,  7.0e-145,  8.0e-145,  9.0e-145,
        1.0e-144,  2.0e-144,  3.0e-144,  4.0e-144,  5.0e-144,  6.0e-144,  7.0e-144,  8.0e-144,  9.0e-144,
        1.0e-143,  2.0e-143,  3.0e-143,  4.0e-143,  5.0e-143,  6.0e-143,  7.0e-143,  8.0e-143,  9.0e-143,
        1.0e-142,  2.0e-142,  3.0e-142,  4.0e-142,  5.0e-142,  6.0e-142,  7.0e-142,  8.0e-142,  9.0e-142,
        1.0e-141,  2.0e-141,  3.0e-141,  4.0e-141,  5.0e-141,  6.0e-141,  7.0e-141,  8.0e-141,  9.0e-141,
        1.0e-140,  2.0e-140,  3.0e-140,  4.0e-140,  5.0e-140,  6.0e-140,  7.0e-140,  8.0e-140,  9.0e-140,
        1.0e-139,  2.0e-139,  3.0e-139,  4.0e-139,  5.0e-139,  6.0e-139,  7.0e-139,  8.0e-139,  9.0e-139,
        1.0e-138,  2.0e-138,  3.0e-138,  4.0e-138,  5.0e-138,  6.0e-138,  7.0e-138,  8.0e-138,  9.0e-138,
        1.0e-137,  2.0e-137,  3.0e-137,  4.0e-137,  5.0e-137,  6.0e-137,  7.0e-137,  8.0e-137,  9.0e-137,
        1.0e-136,  2.0e-136,  3.0e-136,  4.0e-136,  5.0e-136,  6.0e-136,  7.0e-136,  8.0e-136,  9.0e-136,
        1.0e-135,  2.0e-135,  3.0e-135,  4.0e-135,  5.0e-135,  6.0e-135,  7.0e-135,  8.0e-135,  9.0e-135,
        1.0e-134,  2.0e-134,  3.0e-134,  4.0e-134,  5.0e-134,  6.0e-134,  7.0e-134,  8.0e-134,  9.0e-134,
        1.0e-133,  2.0e-133,  3.0e-133,  4.0e-133,  5.0e-133,  6.0e-133,  7.0e-133,  8.0e-133,  9.0e-133,
        1.0e-132,  2.0e-132,  3.0e-132,  4.0e-132,  5.0e-132,  6.0e-132,  7.0e-132,  8.0e-132,  9.0e-132,
        1.0e-131,  2.0e-131,  3.0e-131,  4.0e-131,  5.0e-131,  6.0e-131,  7.0e-131,  8.0e-131,  9.0e-131,
        1.0e-130,  2.0e-130,  3.0e-130,  4.0e-130,  5.0e-130,  6.0e-130,  7.0e-130,  8.0e-130,  9.0e-130,
        1.0e-129,  2.0e-129,  3.0e-129,  4.0e-129,  5.0e-129,  6.0e-129,  7.0e-129,  8.0e-129,  9.0e-129,
        1.0e-128,  2.0e-128,  3.0e-128,  4.0e-128,  5.0e-128,  6.0e-128,  7.0e-128,  8.0e-128,  9.0e-128,
        1.0e-127,  2.0e-127,  3.0e-127,  4.0e-127,  5.0e-127,  6.0e-127,  7.0e-127,  8.0e-127,  9.0e-127,
        1.0e-126,  2.0e-126,  3.0e-126,  4.0e-126,  5.0e-126,  6.0e-126,  7.0e-126,  8.0e-126,  9.0e-126,
        1.0e-125,  2.0e-125,  3.0e-125,  4.0e-125,  5.0e-125,  6.0e-125,  7.0e-125,  8.0e-125,  9.0e-125,
        1.0e-124,  2.0e-124,  3.0e-124,  4.0e-124,  5.0e-124,  6.0e-124,  7.0e-124,  8.0e-124,  9.0e-124,
        1.0e-123,  2.0e-123,  3.0e-123,  4.0e-123,  5.0e-123,  6.0e-123,  7.0e-123,  8.0e-123,  9.0e-123,
        1.0e-122,  2.0e-122,  3.0e-122,  4.0e-122,  5.0e-122,  6.0e-122,  7.0e-122,  8.0e-122,  9.0e-122,
        1.0e-121,  2.0e-121,  3.0e-121,  4.0e-121,  5.0e-121,  6.0e-121,  7.0e-121,  8.0e-121,  9.0e-121,
        1.0e-120,  2.0e-120,  3.0e-120,  4.0e-120,  5.0e-120,  6.0e-120,  7.0e-120,  8.0e-120,  9.0e-120,
        1.0e-119,  2.0e-119,  3.0e-119,  4.0e-119,  5.0e-119,  6.0e-119,  7.0e-119,  8.0e-119,  9.0e-119,
        1.0e-118,  2.0e-118,  3.0e-118,  4.0e-118,  5.0e-118,  6.0e-118,  7.0e-118,  8.0e-118,  9.0e-118,
        1.0e-117,  2.0e-117,  3.0e-117,  4.0e-117,  5.0e-117,  6.0e-117,  7.0e-117,  8.0e-117,  9.0e-117,
        1.0e-116,  2.0e-116,  3.0e-116,  4.0e-116,  5.0e-116,  6.0e-116,  7.0e-116,  8.0e-116,  9.0e-116,
        1.0e-115,  2.0e-115,  3.0e-115,  4.0e-115,  5.0e-115,  6.0e-115,  7.0e-115,  8.0e-115,  9.0e-115,
        1.0e-114,  2.0e-114,  3.0e-114,  4.0e-114,  5.0e-114,  6.0e-114,  7.0e-114,  8.0e-114,  9.0e-114,
        1.0e-113,  2.0e-113,  3.0e-113,  4.0e-113,  5.0e-113,  6.0e-113,  7.0e-113,  8.0e-113,  9.0e-113,
        1.0e-112,  2.0e-112,  3.0e-112,  4.0e-112,  5.0e-112,  6.0e-112,  7.0e-112,  8.0e-112,  9.0e-112,
        1.0e-111,  2.0e-111,  3.0e-111,  4.0e-111,  5.0e-111,  6.0e-111,  7.0e-111,  8.0e-111,  9.0e-111,
        1.0e-110,  2.0e-110,  3.0e-110,  4.0e-110,  5.0e-110,  6.0e-110,  7.0e-110,  8.0e-110,  9.0e-110,
        1.0e-109,  2.0e-109,  3.0e-109,  4.0e-109,  5.0e-109,  6.0e-109,  7.0e-109,  8.0e-109,  9.0e-109,
        1.0e-108,  2.0e-108,  3.0e-108,  4.0e-108,  5.0e-108,  6.0e-108,  7.0e-108,  8.0e-108,  9.0e-108,
        1.0e-107,  2.0e-107,  3.0e-107,  4.0e-107,  5.0e-107,  6.0e-107,  7.0e-107,  8.0e-107,  9.0e-107,
        1.0e-106,  2.0e-106,  3.0e-106,  4.0e-106,  5.0e-106,  6.0e-106,  7.0e-106,  8.0e-106,  9.0e-106,
        1.0e-105,  2.0e-105,  3.0e-105,  4.0e-105,  5.0e-105,  6.0e-105,  7.0e-105,  8.0e-105,  9.0e-105,
        1.0e-104,  2.0e-104,  3.0e-104,  4.0e-104,  5.0e-104,  6.0e-104,  7.0e-104,  8.0e-104,  9.0e-104,
        1.0e-103,  2.0e-103,  3.0e-103,  4.0e-103,  5.0e-103,  6.0e-103,  7.0e-103,  8.0e-103,  9.0e-103,
        1.0e-102,  2.0e-102,  3.0e-102,  4.0e-102,  5.0e-102,  6.0e-102,  7.0e-102,  8.0e-102,  9.0e-102,
        1.0e-101,  2.0e-101,  3.0e-101,  4.0e-101,  5.0e-101,  6.0e-101,  7.0e-101,  8.0e-101,  9.0e-101,
        1.0e-100,  2.0e-100,  3.0e-100,  4.0e-100,  5.0e-100,  6.0e-100,  7.0e-100,  8.0e-100,  9.0e-100,
        1.0e-099,  2.0e-099,  3.0e-099,  4.0e-099,  5.0e-099,  6.0e-099,  7.0e-099,  8.0e-099,  9.0e-099,
        1.0e-098,  2.0e-098,  3.0e-098,  4.0e-098,  5.0e-098,  6.0e-098,  7.0e-098,  8.0e-098,  9.0e-098,
        1.0e-097,  2.0e-097,  3.0e-097,  4.0e-097,  5.0e-097,  6.0e-097,  7.0e-097,  8.0e-097,  9.0e-097,
        1.0e-096,  2.0e-096,  3.0e-096,  4.0e-096,  5.0e-096,  6.0e-096,  7.0e-096,  8.0e-096,  9.0e-096,
        1.0e-095,  2.0e-095,  3.0e-095,  4.0e-095,  5.0e-095,  6.0e-095,  7.0e-095,  8.0e-095,  9.0e-095,
        1.0e-094,  2.0e-094,  3.0e-094,  4.0e-094,  5.0e-094,  6.0e-094,  7.0e-094,  8.0e-094,  9.0e-094,
        1.0e-093,  2.0e-093,  3.0e-093,  4.0e-093,  5.0e-093,  6.0e-093,  7.0e-093,  8.0e-093,  9.0e-093,
        1.0e-092,  2.0e-092,  3.0e-092,  4.0e-092,  5.0e-092,  6.0e-092,  7.0e-092,  8.0e-092,  9.0e-092,
        1.0e-091,  2.0e-091,  3.0e-091,  4.0e-091,  5.0e-091,  6.0e-091,  7.0e-091,  8.0e-091,  9.0e-091,
        1.0e-090,  2.0e-090,  3.0e-090,  4.0e-090,  5.0e-090,  6.0e-090,  7.0e-090,  8.0e-090,  9.0e-090,
        1.0e-089,  2.0e-089,  3.0e-089,  4.0e-089,  5.0e-089,  6.0e-089,  7.0e-089,  8.0e-089,  9.0e-089,
        1.0e-088,  2.0e-088,  3.0e-088,  4.0e-088,  5.0e-088,  6.0e-088,  7.0e-088,  8.0e-088,  9.0e-088,
        1.0e-087,  2.0e-087,  3.0e-087,  4.0e-087,  5.0e-087,  6.0e-087,  7.0e-087,  8.0e-087,  9.0e-087,
        1.0e-086,  2.0e-086,  3.0e-086,  4.0e-086,  5.0e-086,  6.0e-086,  7.0e-086,  8.0e-086,  9.0e-086,
        1.0e-085,  2.0e-085,  3.0e-085,  4.0e-085,  5.0e-085,  6.0e-085,  7.0e-085,  8.0e-085,  9.0e-085,
        1.0e-084,  2.0e-084,  3.0e-084,  4.0e-084,  5.0e-084,  6.0e-084,  7.0e-084,  8.0e-084,  9.0e-084,
        1.0e-083,  2.0e-083,  3.0e-083,  4.0e-083,  5.0e-083,  6.0e-083,  7.0e-083,  8.0e-083,  9.0e-083,
        1.0e-082,  2.0e-082,  3.0e-082,  4.0e-082,  5.0e-082,  6.0e-082,  7.0e-082,  8.0e-082,  9.0e-082,
        1.0e-081,  2.0e-081,  3.0e-081,  4.0e-081,  5.0e-081,  6.0e-081,  7.0e-081,  8.0e-081,  9.0e-081,
        1.0e-080,  2.0e-080,  3.0e-080,  4.0e-080,  5.0e-080,  6.0e-080,  7.0e-080,  8.0e-080,  9.0e-080,
        1.0e-079,  2.0e-079,  3.0e-079,  4.0e-079,  5.0e-079,  6.0e-079,  7.0e-079,  8.0e-079,  9.0e-079,
        1.0e-078,  2.0e-078,  3.0e-078,  4.0e-078,  5.0e-078,  6.0e-078,  7.0e-078,  8.0e-078,  9.0e-078,
        1.0e-077,  2.0e-077,  3.0e-077,  4.0e-077,  5.0e-077,  6.0e-077,  7.0e-077,  8.0e-077,  9.0e-077,
        1.0e-076,  2.0e-076,  3.0e-076,  4.0e-076,  5.0e-076,  6.0e-076,  7.0e-076,  8.0e-076,  9.0e-076,
        1.0e-075,  2.0e-075,  3.0e-075,  4.0e-075,  5.0e-075,  6.0e-075,  7.0e-075,  8.0e-075,  9.0e-075,
        1.0e-074,  2.0e-074,  3.0e-074,  4.0e-074,  5.0e-074,  6.0e-074,  7.0e-074,  8.0e-074,  9.0e-074,
        1.0e-073,  2.0e-073,  3.0e-073,  4.0e-073,  5.0e-073,  6.0e-073,  7.0e-073,  8.0e-073,  9.0e-073,
        1.0e-072,  2.0e-072,  3.0e-072,  4.0e-072,  5.0e-072,  6.0e-072,  7.0e-072,  8.0e-072,  9.0e-072,
        1.0e-071,  2.0e-071,  3.0e-071,  4.0e-071,  5.0e-071,  6.0e-071,  7.0e-071,  8.0e-071,  9.0e-071,
        1.0e-070,  2.0e-070,  3.0e-070,  4.0e-070,  5.0e-070,  6.0e-070,  7.0e-070,  8.0e-070,  9.0e-070,
        1.0e-069,  2.0e-069,  3.0e-069,  4.0e-069,  5.0e-069,  6.0e-069,  7.0e-069,  8.0e-069,  9.0e-069,
        1.0e-068,  2.0e-068,  3.0e-068,  4.0e-068,  5.0e-068,  6.0e-068,  7.0e-068,  8.0e-068,  9.0e-068,
        1.0e-067,  2.0e-067,  3.0e-067,  4.0e-067,  5.0e-067,  6.0e-067,  7.0e-067,  8.0e-067,  9.0e-067,
        1.0e-066,  2.0e-066,  3.0e-066,  4.0e-066,  5.0e-066,  6.0e-066,  7.0e-066,  8.0e-066,  9.0e-066,
        1.0e-065,  2.0e-065,  3.0e-065,  4.0e-065,  5.0e-065,  6.0e-065,  7.0e-065,  8.0e-065,  9.0e-065,
        1.0e-064,  2.0e-064,  3.0e-064,  4.0e-064,  5.0e-064,  6.0e-064,  7.0e-064,  8.0e-064,  9.0e-064,
        1.0e-063,  2.0e-063,  3.0e-063,  4.0e-063,  5.0e-063,  6.0e-063,  7.0e-063,  8.0e-063,  9.0e-063,
        1.0e-062,  2.0e-062,  3.0e-062,  4.0e-062,  5.0e-062,  6.0e-062,  7.0e-062,  8.0e-062,  9.0e-062,
        1.0e-061,  2.0e-061,  3.0e-061,  4.0e-061,  5.0e-061,  6.0e-061,  7.0e-061,  8.0e-061,  9.0e-061,
        1.0e-060,  2.0e-060,  3.0e-060,  4.0e-060,  5.0e-060,  6.0e-060,  7.0e-060,  8.0e-060,  9.0e-060,
        1.0e-059,  2.0e-059,  3.0e-059,  4.0e-059,  5.0e-059,  6.0e-059,  7.0e-059,  8.0e-059,  9.0e-059,
        1.0e-058,  2.0e-058,  3.0e-058,  4.0e-058,  5.0e-058,  6.0e-058,  7.0e-058,  8.0e-058,  9.0e-058,
        1.0e-057,  2.0e-057,  3.0e-057,  4.0e-057,  5.0e-057,  6.0e-057,  7.0e-057,  8.0e-057,  9.0e-057,
        1.0e-056,  2.0e-056,  3.0e-056,  4.0e-056,  5.0e-056,  6.0e-056,  7.0e-056,  8.0e-056,  9.0e-056,
        1.0e-055,  2.0e-055,  3.0e-055,  4.0e-055,  5.0e-055,  6.0e-055,  7.0e-055,  8.0e-055,  9.0e-055,
        1.0e-054,  2.0e-054,  3.0e-054,  4.0e-054,  5.0e-054,  6.0e-054,  7.0e-054,  8.0e-054,  9.0e-054,
        1.0e-053,  2.0e-053,  3.0e-053,  4.0e-053,  5.0e-053,  6.0e-053,  7.0e-053,  8.0e-053,  9.0e-053,
        1.0e-052,  2.0e-052,  3.0e-052,  4.0e-052,  5.0e-052,  6.0e-052,  7.0e-052,  8.0e-052,  9.0e-052,
        1.0e-051,  2.0e-051,  3.0e-051,  4.0e-051,  5.0e-051,  6.0e-051,  7.0e-051,  8.0e-051,  9.0e-051,
        1.0e-050,  2.0e-050,  3.0e-050,  4.0e-050,  5.0e-050,  6.0e-050,  7.0e-050,  8.0e-050,  9.0e-050,
        1.0e-049,  2.0e-049,  3.0e-049,  4.0e-049,  5.0e-049,  6.0e-049,  7.0e-049,  8.0e-049,  9.0e-049,
        1.0e-048,  2.0e-048,  3.0e-048,  4.0e-048,  5.0e-048,  6.0e-048,  7.0e-048,  8.0e-048,  9.0e-048,
        1.0e-047,  2.0e-047,  3.0e-047,  4.0e-047,  5.0e-047,  6.0e-047,  7.0e-047,  8.0e-047,  9.0e-047,
        1.0e-046,  2.0e-046,  3.0e-046,  4.0e-046,  5.0e-046,  6.0e-046,  7.0e-046,  8.0e-046,  9.0e-046,
        1.0e-045,  2.0e-045,  3.0e-045,  4.0e-045,  5.0e-045,  6.0e-045,  7.0e-045,  8.0e-045,  9.0e-045,
        1.0e-044,  2.0e-044,  3.0e-044,  4.0e-044,  5.0e-044,  6.0e-044,  7.0e-044,  8.0e-044,  9.0e-044,
        1.0e-043,  2.0e-043,  3.0e-043,  4.0e-043,  5.0e-043,  6.0e-043,  7.0e-043,  8.0e-043,  9.0e-043,
        1.0e-042,  2.0e-042,  3.0e-042,  4.0e-042,  5.0e-042,  6.0e-042,  7.0e-042,  8.0e-042,  9.0e-042,
        1.0e-041,  2.0e-041,  3.0e-041,  4.0e-041,  5.0e-041,  6.0e-041,  7.0e-041,  8.0e-041,  9.0e-041,
        1.0e-040,  2.0e-040,  3.0e-040,  4.0e-040,  5.0e-040,  6.0e-040,  7.0e-040,  8.0e-040,  9.0e-040,
        1.0e-039,  2.0e-039,  3.0e-039,  4.0e-039,  5.0e-039,  6.0e-039,  7.0e-039,  8.0e-039,  9.0e-039,
        1.0e-038,  2.0e-038,  3.0e-038,  4.0e-038,  5.0e-038,  6.0e-038,  7.0e-038,  8.0e-038,  9.0e-038,
        1.0e-037,  2.0e-037,  3.0e-037,  4.0e-037,  5.0e-037,  6.0e-037,  7.0e-037,  8.0e-037,  9.0e-037,
        1.0e-036,  2.0e-036,  3.0e-036,  4.0e-036,  5.0e-036,  6.0e-036,  7.0e-036,  8.0e-036,  9.0e-036,
        1.0e-035,  2.0e-035,  3.0e-035,  4.0e-035,  5.0e-035,  6.0e-035,  7.0e-035,  8.0e-035,  9.0e-035,
        1.0e-034,  2.0e-034,  3.0e-034,  4.0e-034,  5.0e-034,  6.0e-034,  7.0e-034,  8.0e-034,  9.0e-034,
        1.0e-033,  2.0e-033,  3.0e-033,  4.0e-033,  5.0e-033,  6.0e-033,  7.0e-033,  8.0e-033,  9.0e-033,
        1.0e-032,  2.0e-032,  3.0e-032,  4.0e-032,  5.0e-032,  6.0e-032,  7.0e-032,  8.0e-032,  9.0e-032,
        1.0e-031,  2.0e-031,  3.0e-031,  4.0e-031,  5.0e-031,  6.0e-031,  7.0e-031,  8.0e-031,  9.0e-031,
        1.0e-030,  2.0e-030,  3.0e-030,  4.0e-030,  5.0e-030,  6.0e-030,  7.0e-030,  8.0e-030,  9.0e-030,
        1.0e-029,  2.0e-029,  3.0e-029,  4.0e-029,  5.0e-029,  6.0e-029,  7.0e-029,  8.0e-029,  9.0e-029,
        1.0e-028,  2.0e-028,  3.0e-028,  4.0e-028,  5.0e-028,  6.0e-028,  7.0e-028,  8.0e-028,  9.0e-028,
        1.0e-027,  2.0e-027,  3.0e-027,  4.0e-027,  5.0e-027,  6.0e-027,  7.0e-027,  8.0e-027,  9.0e-027,
        1.0e-026,  2.0e-026,  3.0e-026,  4.0e-026,  5.0e-026,  6.0e-026,  7.0e-026,  8.0e-026,  9.0e-026,
        1.0e-025,  2.0e-025,  3.0e-025,  4.0e-025,  5.0e-025,  6.0e-025,  7.0e-025,  8.0e-025,  9.0e-025,
        1.0e-024,  2.0e-024,  3.0e-024,  4.0e-024,  5.0e-024,  6.0e-024,  7.0e-024,  8.0e-024,  9.0e-024,
        1.0e-023,  2.0e-023,  3.0e-023,  4.0e-023,  5.0e-023,  6.0e-023,  7.0e-023,  8.0e-023,  9.0e-023,
        1.0e-022,  2.0e-022,  3.0e-022,  4.0e-022,  5.0e-022,  6.0e-022,  7.0e-022,  8.0e-022,  9.0e-022,
        1.0e-021,  2.0e-021,  3.0e-021,  4.0e-021,  5.0e-021,  6.0e-021,  7.0e-021,  8.0e-021,  9.0e-021,
        1.0e-020,  2.0e-020,  3.0e-020,  4.0e-020,  5.0e-020,  6.0e-020,  7.0e-020,  8.0e-020,  9.0e-020,
        1.0e-019,  2.0e-019,  3.0e-019,  4.0e-019,  5.0e-019,  6.0e-019,  7.0e-019,  8.0e-019,  9.0e-019,
        1.0e-018,  2.0e-018,  3.0e-018,  4.0e-018,  5.0e-018,  6.0e-018,  7.0e-018,  8.0e-018,  9.0e-018,
        1.0e-017,  2.0e-017,  3.0e-017,  4.0e-017,  5.0e-017,  6.0e-017,  7.0e-017,  8.0e-017,  9.0e-017,
        1.0e-016,  2.0e-016,  3.0e-016,  4.0e-016,  5.0e-016,  6.0e-016,  7.0e-016,  8.0e-016,  9.0e-016,
        1.0e-015,  2.0e-015,  3.0e-015,  4.0e-015,  5.0e-015,  6.0e-015,  7.0e-015,  8.0e-015,  9.0e-015,
        1.0e-014,  2.0e-014,  3.0e-014,  4.0e-014,  5.0e-014,  6.0e-014,  7.0e-014,  8.0e-014,  9.0e-014,
        1.0e-013,  2.0e-013,  3.0e-013,  4.0e-013,  5.0e-013,  6.0e-013,  7.0e-013,  8.0e-013,  9.0e-013,
        1.0e-012,  2.0e-012,  3.0e-012,  4.0e-012,  5.0e-012,  6.0e-012,  7.0e-012,  8.0e-012,  9.0e-012,
        1.0e-011,  2.0e-011,  3.0e-011,  4.0e-011,  5.0e-011,  6.0e-011,  7.0e-011,  8.0e-011,  9.0e-011,
        1.0e-010,  2.0e-010,  3.0e-010,  4.0e-010,  5.0e-010,  6.0e-010,  7.0e-010,  8.0e-010,  9.0e-010,
        1.0e-009,  2.0e-009,  3.0e-009,  4.0e-009,  5.0e-009,  6.0e-009,  7.0e-009,  8.0e-009,  9.0e-009,
        1.0e-008,  2.0e-008,  3.0e-008,  4.0e-008,  5.0e-008,  6.0e-008,  7.0e-008,  8.0e-008,  9.0e-008,
        1.0e-007,  2.0e-007,  3.0e-007,  4.0e-007,  5.0e-007,  6.0e-007,  7.0e-007,  8.0e-007,  9.0e-007,
        1.0e-006,  2.0e-006,  3.0e-006,  4.0e-006,  5.0e-006,  6.0e-006,  7.0e-006,  8.0e-006,  9.0e-006,
        1.0e-005,  2.0e-005,  3.0e-005,  4.0e-005,  5.0e-005,  6.0e-005,  7.0e-005,  8.0e-005,  9.0e-005,
        1.0e-004,  2.0e-004,  3.0e-004,  4.0e-004,  5.0e-004,  6.0e-004,  7.0e-004,  8.0e-004,  9.0e-004,
        1.0e-003,  2.0e-003,  3.0e-003,  4.0e-003,  5.0e-003,  6.0e-003,  7.0e-003,  8.0e-003,  9.0e-003,
        1.0e-002,  2.0e-002,  3.0e-002,  4.0e-002,  5.0e-002,  6.0e-002,  7.0e-002,  8.0e-002,  9.0e-002,
        1.0e-001,  2.0e-001,  3.0e-001,  4.0e-001,  5.0e-001,  6.0e-001,  7.0e-001,  8.0e-001,  9.0e-001,
        1.0e+000,  2.0e+000,  3.0e+000,  4.0e+000,  5.0e+000,  6.0e+000,  7.0e+000,  8.0e+000,  9.0e+000,
        1.0e+001,  2.0e+001,  3.0e+001,  4.0e+001,  5.0e+001,  6.0e+001,  7.0e+001,  8.0e+001,  9.0e+001,
        1.0e+002,  2.0e+002,  3.0e+002,  4.0e+002,  5.0e+002,  6.0e+002,  7.0e+002,  8.0e+002,  9.0e+002,
        1.0e+003,  2.0e+003,  3.0e+003,  4.0e+003,  5.0e+003,  6.0e+003,  7.0e+003,  8.0e+003,  9.0e+003,
        1.0e+004,  2.0e+004,  3.0e+004,  4.0e+004,  5.0e+004,  6.0e+004,  7.0e+004,  8.0e+004,  9.0e+004,
        1.0e+005,  2.0e+005,  3.0e+005,  4.0e+005,  5.0e+005,  6.0e+005,  7.0e+005,  8.0e+005,  9.0e+005,
        1.0e+006,  2.0e+006,  3.0e+006,  4.0e+006,  5.0e+006,  6.0e+006,  7.0e+006,  8.0e+006,  9.0e+006,
        1.0e+007,  2.0e+007,  3.0e+007,  4.0e+007,  5.0e+007,  6.0e+007,  7.0e+007,  8.0e+007,  9.0e+007,
        1.0e+008,  2.0e+008,  3.0e+008,  4.0e+008,  5.0e+008,  6.0e+008,  7.0e+008,  8.0e+008,  9.0e+008,
        1.0e+009,  2.0e+009,  3.0e+009,  4.0e+009,  5.0e+009,  6.0e+009,  7.0e+009,  8.0e+009,  9.0e+009,
        1.0e+010,  2.0e+010,  3.0e+010,  4.0e+010,  5.0e+010,  6.0e+010,  7.0e+010,  8.0e+010,  9.0e+010,
        1.0e+011,  2.0e+011,  3.0e+011,  4.0e+011,  5.0e+011,  6.0e+011,  7.0e+011,  8.0e+011,  9.0e+011,
        1.0e+012,  2.0e+012,  3.0e+012,  4.0e+012,  5.0e+012,  6.0e+012,  7.0e+012,  8.0e+012,  9.0e+012,
        1.0e+013,  2.0e+013,  3.0e+013,  4.0e+013,  5.0e+013,  6.0e+013,  7.0e+013,  8.0e+013,  9.0e+013,
        1.0e+014,  2.0e+014,  3.0e+014,  4.0e+014,  5.0e+014,  6.0e+014,  7.0e+014,  8.0e+014,  9.0e+014,
        1.0e+015,  2.0e+015,  3.0e+015,  4.0e+015,  5.0e+015,  6.0e+015,  7.0e+015,  8.0e+015,  9.0e+015,
        1.0e+016,  2.0e+016,  3.0e+016,  4.0e+016,  5.0e+016,  6.0e+016,  7.0e+016,  8.0e+016,  9.0e+016,
        1.0e+017,  2.0e+017,  3.0e+017,  4.0e+017,  5.0e+017,  6.0e+017,  7.0e+017,  8.0e+017,  9.0e+017,
        1.0e+018,  2.0e+018,  3.0e+018,  4.0e+018,  5.0e+018,  6.0e+018,  7.0e+018,  8.0e+018,  9.0e+018,
        1.0e+019,  2.0e+019,  3.0e+019,  4.0e+019,  5.0e+019,  6.0e+019,  7.0e+019,  8.0e+019,  9.0e+019,
        1.0e+020,  2.0e+020,  3.0e+020,  4.0e+020,  5.0e+020,  6.0e+020,  7.0e+020,  8.0e+020,  9.0e+020,
        1.0e+021,  2.0e+021,  3.0e+021,  4.0e+021,  5.0e+021,  6.0e+021,  7.0e+021,  8.0e+021,  9.0e+021,
        1.0e+022,  2.0e+022,  3.0e+022,  4.0e+022,  5.0e+022,  6.0e+022,  7.0e+022,  8.0e+022,  9.0e+022,
        1.0e+023,  2.0e+023,  3.0e+023,  4.0e+023,  5.0e+023,  6.0e+023,  7.0e+023,  8.0e+023,  9.0e+023,
        1.0e+024,  2.0e+024,  3.0e+024,  4.0e+024,  5.0e+024,  6.0e+024,  7.0e+024,  8.0e+024,  9.0e+024,
        1.0e+025,  2.0e+025,  3.0e+025,  4.0e+025,  5.0e+025,  6.0e+025,  7.0e+025,  8.0e+025,  9.0e+025,
        1.0e+026,  2.0e+026,  3.0e+026,  4.0e+026,  5.0e+026,  6.0e+026,  7.0e+026,  8.0e+026,  9.0e+026,
        1.0e+027,  2.0e+027,  3.0e+027,  4.0e+027,  5.0e+027,  6.0e+027,  7.0e+027,  8.0e+027,  9.0e+027,
        1.0e+028,  2.0e+028,  3.0e+028,  4.0e+028,  5.0e+028,  6.0e+028,  7.0e+028,  8.0e+028,  9.0e+028,
        1.0e+029,  2.0e+029,  3.0e+029,  4.0e+029,  5.0e+029,  6.0e+029,  7.0e+029,  8.0e+029,  9.0e+029,
        1.0e+030,  2.0e+030,  3.0e+030,  4.0e+030,  5.0e+030,  6.0e+030,  7.0e+030,  8.0e+030,  9.0e+030,
        1.0e+031,  2.0e+031,  3.0e+031,  4.0e+031,  5.0e+031,  6.0e+031,  7.0e+031,  8.0e+031,  9.0e+031,
        1.0e+032,  2.0e+032,  3.0e+032,  4.0e+032,  5.0e+032,  6.0e+032,  7.0e+032,  8.0e+032,  9.0e+032,
        1.0e+033,  2.0e+033,  3.0e+033,  4.0e+033,  5.0e+033,  6.0e+033,  7.0e+033,  8.0e+033,  9.0e+033,
        1.0e+034,  2.0e+034,  3.0e+034,  4.0e+034,  5.0e+034,  6.0e+034,  7.0e+034,  8.0e+034,  9.0e+034,
        1.0e+035,  2.0e+035,  3.0e+035,  4.0e+035,  5.0e+035,  6.0e+035,  7.0e+035,  8.0e+035,  9.0e+035,
        1.0e+036,  2.0e+036,  3.0e+036,  4.0e+036,  5.0e+036,  6.0e+036,  7.0e+036,  8.0e+036,  9.0e+036,
        1.0e+037,  2.0e+037,  3.0e+037,  4.0e+037,  5.0e+037,  6.0e+037,  7.0e+037,  8.0e+037,  9.0e+037,
        1.0e+038,  2.0e+038,  3.0e+038,  4.0e+038,  5.0e+038,  6.0e+038,  7.0e+038,  8.0e+038,  9.0e+038,
        1.0e+039,  2.0e+039,  3.0e+039,  4.0e+039,  5.0e+039,  6.0e+039,  7.0e+039,  8.0e+039,  9.0e+039,
        1.0e+040,  2.0e+040,  3.0e+040,  4.0e+040,  5.0e+040,  6.0e+040,  7.0e+040,  8.0e+040,  9.0e+040,
        1.0e+041,  2.0e+041,  3.0e+041,  4.0e+041,  5.0e+041,  6.0e+041,  7.0e+041,  8.0e+041,  9.0e+041,
        1.0e+042,  2.0e+042,  3.0e+042,  4.0e+042,  5.0e+042,  6.0e+042,  7.0e+042,  8.0e+042,  9.0e+042,
        1.0e+043,  2.0e+043,  3.0e+043,  4.0e+043,  5.0e+043,  6.0e+043,  7.0e+043,  8.0e+043,  9.0e+043,
        1.0e+044,  2.0e+044,  3.0e+044,  4.0e+044,  5.0e+044,  6.0e+044,  7.0e+044,  8.0e+044,  9.0e+044,
        1.0e+045,  2.0e+045,  3.0e+045,  4.0e+045,  5.0e+045,  6.0e+045,  7.0e+045,  8.0e+045,  9.0e+045,
        1.0e+046,  2.0e+046,  3.0e+046,  4.0e+046,  5.0e+046,  6.0e+046,  7.0e+046,  8.0e+046,  9.0e+046,
        1.0e+047,  2.0e+047,  3.0e+047,  4.0e+047,  5.0e+047,  6.0e+047,  7.0e+047,  8.0e+047,  9.0e+047,
        1.0e+048,  2.0e+048,  3.0e+048,  4.0e+048,  5.0e+048,  6.0e+048,  7.0e+048,  8.0e+048,  9.0e+048,
        1.0e+049,  2.0e+049,  3.0e+049,  4.0e+049,  5.0e+049,  6.0e+049,  7.0e+049,  8.0e+049,  9.0e+049,
        1.0e+050,  2.0e+050,  3.0e+050,  4.0e+050,  5.0e+050,  6.0e+050,  7.0e+050,  8.0e+050,  9.0e+050,
        1.0e+051,  2.0e+051,  3.0e+051,  4.0e+051,  5.0e+051,  6.0e+051,  7.0e+051,  8.0e+051,  9.0e+051,
        1.0e+052,  2.0e+052,  3.0e+052,  4.0e+052,  5.0e+052,  6.0e+052,  7.0e+052,  8.0e+052,  9.0e+052,
        1.0e+053,  2.0e+053,  3.0e+053,  4.0e+053,  5.0e+053,  6.0e+053,  7.0e+053,  8.0e+053,  9.0e+053,
        1.0e+054,  2.0e+054,  3.0e+054,  4.0e+054,  5.0e+054,  6.0e+054,  7.0e+054,  8.0e+054,  9.0e+054,
        1.0e+055,  2.0e+055,  3.0e+055,  4.0e+055,  5.0e+055,  6.0e+055,  7.0e+055,  8.0e+055,  9.0e+055,
        1.0e+056,  2.0e+056,  3.0e+056,  4.0e+056,  5.0e+056,  6.0e+056,  7.0e+056,  8.0e+056,  9.0e+056,
        1.0e+057,  2.0e+057,  3.0e+057,  4.0e+057,  5.0e+057,  6.0e+057,  7.0e+057,  8.0e+057,  9.0e+057,
        1.0e+058,  2.0e+058,  3.0e+058,  4.0e+058,  5.0e+058,  6.0e+058,  7.0e+058,  8.0e+058,  9.0e+058,
        1.0e+059,  2.0e+059,  3.0e+059,  4.0e+059,  5.0e+059,  6.0e+059,  7.0e+059,  8.0e+059,  9.0e+059,
        1.0e+060,  2.0e+060,  3.0e+060,  4.0e+060,  5.0e+060,  6.0e+060,  7.0e+060,  8.0e+060,  9.0e+060,
        1.0e+061,  2.0e+061,  3.0e+061,  4.0e+061,  5.0e+061,  6.0e+061,  7.0e+061,  8.0e+061,  9.0e+061,
        1.0e+062,  2.0e+062,  3.0e+062,  4.0e+062,  5.0e+062,  6.0e+062,  7.0e+062,  8.0e+062,  9.0e+062,
        1.0e+063,  2.0e+063,  3.0e+063,  4.0e+063,  5.0e+063,  6.0e+063,  7.0e+063,  8.0e+063,  9.0e+063,
        1.0e+064,  2.0e+064,  3.0e+064,  4.0e+064,  5.0e+064,  6.0e+064,  7.0e+064,  8.0e+064,  9.0e+064,
        1.0e+065,  2.0e+065,  3.0e+065,  4.0e+065,  5.0e+065,  6.0e+065,  7.0e+065,  8.0e+065,  9.0e+065,
        1.0e+066,  2.0e+066,  3.0e+066,  4.0e+066,  5.0e+066,  6.0e+066,  7.0e+066,  8.0e+066,  9.0e+066,
        1.0e+067,  2.0e+067,  3.0e+067,  4.0e+067,  5.0e+067,  6.0e+067,  7.0e+067,  8.0e+067,  9.0e+067,
        1.0e+068,  2.0e+068,  3.0e+068,  4.0e+068,  5.0e+068,  6.0e+068,  7.0e+068,  8.0e+068,  9.0e+068,
        1.0e+069,  2.0e+069,  3.0e+069,  4.0e+069,  5.0e+069,  6.0e+069,  7.0e+069,  8.0e+069,  9.0e+069,
        1.0e+070,  2.0e+070,  3.0e+070,  4.0e+070,  5.0e+070,  6.0e+070,  7.0e+070,  8.0e+070,  9.0e+070,
        1.0e+071,  2.0e+071,  3.0e+071,  4.0e+071,  5.0e+071,  6.0e+071,  7.0e+071,  8.0e+071,  9.0e+071,
        1.0e+072,  2.0e+072,  3.0e+072,  4.0e+072,  5.0e+072,  6.0e+072,  7.0e+072,  8.0e+072,  9.0e+072,
        1.0e+073,  2.0e+073,  3.0e+073,  4.0e+073,  5.0e+073,  6.0e+073,  7.0e+073,  8.0e+073,  9.0e+073,
        1.0e+074,  2.0e+074,  3.0e+074,  4.0e+074,  5.0e+074,  6.0e+074,  7.0e+074,  8.0e+074,  9.0e+074,
        1.0e+075,  2.0e+075,  3.0e+075,  4.0e+075,  5.0e+075,  6.0e+075,  7.0e+075,  8.0e+075,  9.0e+075,
        1.0e+076,  2.0e+076,  3.0e+076,  4.0e+076,  5.0e+076,  6.0e+076,  7.0e+076,  8.0e+076,  9.0e+076,
        1.0e+077,  2.0e+077,  3.0e+077,  4.0e+077,  5.0e+077,  6.0e+077,  7.0e+077,  8.0e+077,  9.0e+077,
        1.0e+078,  2.0e+078,  3.0e+078,  4.0e+078,  5.0e+078,  6.0e+078,  7.0e+078,  8.0e+078,  9.0e+078,
        1.0e+079,  2.0e+079,  3.0e+079,  4.0e+079,  5.0e+079,  6.0e+079,  7.0e+079,  8.0e+079,  9.0e+079,
        1.0e+080,  2.0e+080,  3.0e+080,  4.0e+080,  5.0e+080,  6.0e+080,  7.0e+080,  8.0e+080,  9.0e+080,
        1.0e+081,  2.0e+081,  3.0e+081,  4.0e+081,  5.0e+081,  6.0e+081,  7.0e+081,  8.0e+081,  9.0e+081,
        1.0e+082,  2.0e+082,  3.0e+082,  4.0e+082,  5.0e+082,  6.0e+082,  7.0e+082,  8.0e+082,  9.0e+082,
        1.0e+083,  2.0e+083,  3.0e+083,  4.0e+083,  5.0e+083,  6.0e+083,  7.0e+083,  8.0e+083,  9.0e+083,
        1.0e+084,  2.0e+084,  3.0e+084,  4.0e+084,  5.0e+084,  6.0e+084,  7.0e+084,  8.0e+084,  9.0e+084,
        1.0e+085,  2.0e+085,  3.0e+085,  4.0e+085,  5.0e+085,  6.0e+085,  7.0e+085,  8.0e+085,  9.0e+085,
        1.0e+086,  2.0e+086,  3.0e+086,  4.0e+086,  5.0e+086,  6.0e+086,  7.0e+086,  8.0e+086,  9.0e+086,
        1.0e+087,  2.0e+087,  3.0e+087,  4.0e+087,  5.0e+087,  6.0e+087,  7.0e+087,  8.0e+087,  9.0e+087,
        1.0e+088,  2.0e+088,  3.0e+088,  4.0e+088,  5.0e+088,  6.0e+088,  7.0e+088,  8.0e+088,  9.0e+088,
        1.0e+089,  2.0e+089,  3.0e+089,  4.0e+089,  5.0e+089,  6.0e+089,  7.0e+089,  8.0e+089,  9.0e+089,
        1.0e+090,  2.0e+090,  3.0e+090,  4.0e+090,  5.0e+090,  6.0e+090,  7.0e+090,  8.0e+090,  9.0e+090,
        1.0e+091,  2.0e+091,  3.0e+091,  4.0e+091,  5.0e+091,  6.0e+091,  7.0e+091,  8.0e+091,  9.0e+091,
        1.0e+092,  2.0e+092,  3.0e+092,  4.0e+092,  5.0e+092,  6.0e+092,  7.0e+092,  8.0e+092,  9.0e+092,
        1.0e+093,  2.0e+093,  3.0e+093,  4.0e+093,  5.0e+093,  6.0e+093,  7.0e+093,  8.0e+093,  9.0e+093,
        1.0e+094,  2.0e+094,  3.0e+094,  4.0e+094,  5.0e+094,  6.0e+094,  7.0e+094,  8.0e+094,  9.0e+094,
        1.0e+095,  2.0e+095,  3.0e+095,  4.0e+095,  5.0e+095,  6.0e+095,  7.0e+095,  8.0e+095,  9.0e+095,
        1.0e+096,  2.0e+096,  3.0e+096,  4.0e+096,  5.0e+096,  6.0e+096,  7.0e+096,  8.0e+096,  9.0e+096,
        1.0e+097,  2.0e+097,  3.0e+097,  4.0e+097,  5.0e+097,  6.0e+097,  7.0e+097,  8.0e+097,  9.0e+097,
        1.0e+098,  2.0e+098,  3.0e+098,  4.0e+098,  5.0e+098,  6.0e+098,  7.0e+098,  8.0e+098,  9.0e+098,
        1.0e+099,  2.0e+099,  3.0e+099,  4.0e+099,  5.0e+099,  6.0e+099,  7.0e+099,  8.0e+099,  9.0e+099,
        1.0e+100,  2.0e+100,  3.0e+100,  4.0e+100,  5.0e+100,  6.0e+100,  7.0e+100,  8.0e+100,  9.0e+100,
        1.0e+101,  2.0e+101,  3.0e+101,  4.0e+101,  5.0e+101,  6.0e+101,  7.0e+101,  8.0e+101,  9.0e+101,
        1.0e+102,  2.0e+102,  3.0e+102,  4.0e+102,  5.0e+102,  6.0e+102,  7.0e+102,  8.0e+102,  9.0e+102,
        1.0e+103,  2.0e+103,  3.0e+103,  4.0e+103,  5.0e+103,  6.0e+103,  7.0e+103,  8.0e+103,  9.0e+103,
        1.0e+104,  2.0e+104,  3.0e+104,  4.0e+104,  5.0e+104,  6.0e+104,  7.0e+104,  8.0e+104,  9.0e+104,
        1.0e+105,  2.0e+105,  3.0e+105,  4.0e+105,  5.0e+105,  6.0e+105,  7.0e+105,  8.0e+105,  9.0e+105,
        1.0e+106,  2.0e+106,  3.0e+106,  4.0e+106,  5.0e+106,  6.0e+106,  7.0e+106,  8.0e+106,  9.0e+106,
        1.0e+107,  2.0e+107,  3.0e+107,  4.0e+107,  5.0e+107,  6.0e+107,  7.0e+107,  8.0e+107,  9.0e+107,
        1.0e+108,  2.0e+108,  3.0e+108,  4.0e+108,  5.0e+108,  6.0e+108,  7.0e+108,  8.0e+108,  9.0e+108,
        1.0e+109,  2.0e+109,  3.0e+109,  4.0e+109,  5.0e+109,  6.0e+109,  7.0e+109,  8.0e+109,  9.0e+109,
        1.0e+110,  2.0e+110,  3.0e+110,  4.0e+110,  5.0e+110,  6.0e+110,  7.0e+110,  8.0e+110,  9.0e+110,
        1.0e+111,  2.0e+111,  3.0e+111,  4.0e+111,  5.0e+111,  6.0e+111,  7.0e+111,  8.0e+111,  9.0e+111,
        1.0e+112,  2.0e+112,  3.0e+112,  4.0e+112,  5.0e+112,  6.0e+112,  7.0e+112,  8.0e+112,  9.0e+112,
        1.0e+113,  2.0e+113,  3.0e+113,  4.0e+113,  5.0e+113,  6.0e+113,  7.0e+113,  8.0e+113,  9.0e+113,
        1.0e+114,  2.0e+114,  3.0e+114,  4.0e+114,  5.0e+114,  6.0e+114,  7.0e+114,  8.0e+114,  9.0e+114,
        1.0e+115,  2.0e+115,  3.0e+115,  4.0e+115,  5.0e+115,  6.0e+115,  7.0e+115,  8.0e+115,  9.0e+115,
        1.0e+116,  2.0e+116,  3.0e+116,  4.0e+116,  5.0e+116,  6.0e+116,  7.0e+116,  8.0e+116,  9.0e+116,
        1.0e+117,  2.0e+117,  3.0e+117,  4.0e+117,  5.0e+117,  6.0e+117,  7.0e+117,  8.0e+117,  9.0e+117,
        1.0e+118,  2.0e+118,  3.0e+118,  4.0e+118,  5.0e+118,  6.0e+118,  7.0e+118,  8.0e+118,  9.0e+118,
        1.0e+119,  2.0e+119,  3.0e+119,  4.0e+119,  5.0e+119,  6.0e+119,  7.0e+119,  8.0e+119,  9.0e+119,
        1.0e+120,  2.0e+120,  3.0e+120,  4.0e+120,  5.0e+120,  6.0e+120,  7.0e+120,  8.0e+120,  9.0e+120,
        1.0e+121,  2.0e+121,  3.0e+121,  4.0e+121,  5.0e+121,  6.0e+121,  7.0e+121,  8.0e+121,  9.0e+121,
        1.0e+122,  2.0e+122,  3.0e+122,  4.0e+122,  5.0e+122,  6.0e+122,  7.0e+122,  8.0e+122,  9.0e+122,
        1.0e+123,  2.0e+123,  3.0e+123,  4.0e+123,  5.0e+123,  6.0e+123,  7.0e+123,  8.0e+123,  9.0e+123,
        1.0e+124,  2.0e+124,  3.0e+124,  4.0e+124,  5.0e+124,  6.0e+124,  7.0e+124,  8.0e+124,  9.0e+124,
        1.0e+125,  2.0e+125,  3.0e+125,  4.0e+125,  5.0e+125,  6.0e+125,  7.0e+125,  8.0e+125,  9.0e+125,
        1.0e+126,  2.0e+126,  3.0e+126,  4.0e+126,  5.0e+126,  6.0e+126,  7.0e+126,  8.0e+126,  9.0e+126,
        1.0e+127,  2.0e+127,  3.0e+127,  4.0e+127,  5.0e+127,  6.0e+127,  7.0e+127,  8.0e+127,  9.0e+127,
        1.0e+128,  2.0e+128,  3.0e+128,  4.0e+128,  5.0e+128,  6.0e+128,  7.0e+128,  8.0e+128,  9.0e+128,
        1.0e+129,  2.0e+129,  3.0e+129,  4.0e+129,  5.0e+129,  6.0e+129,  7.0e+129,  8.0e+129,  9.0e+129,
        1.0e+130,  2.0e+130,  3.0e+130,  4.0e+130,  5.0e+130,  6.0e+130,  7.0e+130,  8.0e+130,  9.0e+130,
        1.0e+131,  2.0e+131,  3.0e+131,  4.0e+131,  5.0e+131,  6.0e+131,  7.0e+131,  8.0e+131,  9.0e+131,
        1.0e+132,  2.0e+132,  3.0e+132,  4.0e+132,  5.0e+132,  6.0e+132,  7.0e+132,  8.0e+132,  9.0e+132,
        1.0e+133,  2.0e+133,  3.0e+133,  4.0e+133,  5.0e+133,  6.0e+133,  7.0e+133,  8.0e+133,  9.0e+133,
        1.0e+134,  2.0e+134,  3.0e+134,  4.0e+134,  5.0e+134,  6.0e+134,  7.0e+134,  8.0e+134,  9.0e+134,
        1.0e+135,  2.0e+135,  3.0e+135,  4.0e+135,  5.0e+135,  6.0e+135,  7.0e+135,  8.0e+135,  9.0e+135,
        1.0e+136,  2.0e+136,  3.0e+136,  4.0e+136,  5.0e+136,  6.0e+136,  7.0e+136,  8.0e+136,  9.0e+136,
        1.0e+137,  2.0e+137,  3.0e+137,  4.0e+137,  5.0e+137,  6.0e+137,  7.0e+137,  8.0e+137,  9.0e+137,
        1.0e+138,  2.0e+138,  3.0e+138,  4.0e+138,  5.0e+138,  6.0e+138,  7.0e+138,  8.0e+138,  9.0e+138,
        1.0e+139,  2.0e+139,  3.0e+139,  4.0e+139,  5.0e+139,  6.0e+139,  7.0e+139,  8.0e+139,  9.0e+139,
        1.0e+140,  2.0e+140,  3.0e+140,  4.0e+140,  5.0e+140,  6.0e+140,  7.0e+140,  8.0e+140,  9.0e+140,
        1.0e+141,  2.0e+141,  3.0e+141,  4.0e+141,  5.0e+141,  6.0e+141,  7.0e+141,  8.0e+141,  9.0e+141,
        1.0e+142,  2.0e+142,  3.0e+142,  4.0e+142,  5.0e+142,  6.0e+142,  7.0e+142,  8.0e+142,  9.0e+142,
        1.0e+143,  2.0e+143,  3.0e+143,  4.0e+143,  5.0e+143,  6.0e+143,  7.0e+143,  8.0e+143,  9.0e+143,
        1.0e+144,  2.0e+144,  3.0e+144,  4.0e+144,  5.0e+144,  6.0e+144,  7.0e+144,  8.0e+144,  9.0e+144,
        1.0e+145,  2.0e+145,  3.0e+145,  4.0e+145,  5.0e+145,  6.0e+145,  7.0e+145,  8.0e+145,  9.0e+145,
        1.0e+146,  2.0e+146,  3.0e+146,  4.0e+146,  5.0e+146,  6.0e+146,  7.0e+146,  8.0e+146,  9.0e+146,
        1.0e+147,  2.0e+147,  3.0e+147,  4.0e+147,  5.0e+147,  6.0e+147,  7.0e+147,  8.0e+147,  9.0e+147,
        1.0e+148,  2.0e+148,  3.0e+148,  4.0e+148,  5.0e+148,  6.0e+148,  7.0e+148,  8.0e+148,  9.0e+148,
        1.0e+149,  2.0e+149,  3.0e+149,  4.0e+149,  5.0e+149,  6.0e+149,  7.0e+149,  8.0e+149,  9.0e+149,
        1.0e+150,  2.0e+150,  3.0e+150,  4.0e+150,  5.0e+150,  6.0e+150,  7.0e+150,  8.0e+150,  9.0e+150,
        1.0e+151,  2.0e+151,  3.0e+151,  4.0e+151,  5.0e+151,  6.0e+151,  7.0e+151,  8.0e+151,  9.0e+151,
        1.0e+152,  2.0e+152,  3.0e+152,  4.0e+152,  5.0e+152,  6.0e+152,  7.0e+152,  8.0e+152,  9.0e+152,
        1.0e+153,  2.0e+153,  3.0e+153,  4.0e+153,  5.0e+153,  6.0e+153,  7.0e+153,  8.0e+153,  9.0e+153,
        1.0e+154,  2.0e+154,  3.0e+154,  4.0e+154,  5.0e+154,  6.0e+154,  7.0e+154,  8.0e+154,  9.0e+154,
        1.0e+155,  2.0e+155,  3.0e+155,  4.0e+155,  5.0e+155,  6.0e+155,  7.0e+155,  8.0e+155,  9.0e+155,
        1.0e+156,  2.0e+156,  3.0e+156,  4.0e+156,  5.0e+156,  6.0e+156,  7.0e+156,  8.0e+156,  9.0e+156,
        1.0e+157,  2.0e+157,  3.0e+157,  4.0e+157,  5.0e+157,  6.0e+157,  7.0e+157,  8.0e+157,  9.0e+157,
        1.0e+158,  2.0e+158,  3.0e+158,  4.0e+158,  5.0e+158,  6.0e+158,  7.0e+158,  8.0e+158,  9.0e+158,
        1.0e+159,  2.0e+159,  3.0e+159,  4.0e+159,  5.0e+159,  6.0e+159,  7.0e+159,  8.0e+159,  9.0e+159,
        1.0e+160,  2.0e+160,  3.0e+160,  4.0e+160,  5.0e+160,  6.0e+160,  7.0e+160,  8.0e+160,  9.0e+160,
        1.0e+161,  2.0e+161,  3.0e+161,  4.0e+161,  5.0e+161,  6.0e+161,  7.0e+161,  8.0e+161,  9.0e+161,
        1.0e+162,  2.0e+162,  3.0e+162,  4.0e+162,  5.0e+162,  6.0e+162,  7.0e+162,  8.0e+162,  9.0e+162,
        1.0e+163,  2.0e+163,  3.0e+163,  4.0e+163,  5.0e+163,  6.0e+163,  7.0e+163,  8.0e+163,  9.0e+163,
        1.0e+164,  2.0e+164,  3.0e+164,  4.0e+164,  5.0e+164,  6.0e+164,  7.0e+164,  8.0e+164,  9.0e+164,
        1.0e+165,  2.0e+165,  3.0e+165,  4.0e+165,  5.0e+165,  6.0e+165,  7.0e+165,  8.0e+165,  9.0e+165,
        1.0e+166,  2.0e+166,  3.0e+166,  4.0e+166,  5.0e+166,  6.0e+166,  7.0e+166,  8.0e+166,  9.0e+166,
        1.0e+167,  2.0e+167,  3.0e+167,  4.0e+167,  5.0e+167,  6.0e+167,  7.0e+167,  8.0e+167,  9.0e+167,
        1.0e+168,  2.0e+168,  3.0e+168,  4.0e+168,  5.0e+168,  6.0e+168,  7.0e+168,  8.0e+168,  9.0e+168,
        1.0e+169,  2.0e+169,  3.0e+169,  4.0e+169,  5.0e+169,  6.0e+169,  7.0e+169,  8.0e+169,  9.0e+169,
        1.0e+170,  2.0e+170,  3.0e+170,  4.0e+170,  5.0e+170,  6.0e+170,  7.0e+170,  8.0e+170,  9.0e+170,
        1.0e+171,  2.0e+171,  3.0e+171,  4.0e+171,  5.0e+171,  6.0e+171,  7.0e+171,  8.0e+171,  9.0e+171,
        1.0e+172,  2.0e+172,  3.0e+172,  4.0e+172,  5.0e+172,  6.0e+172,  7.0e+172,  8.0e+172,  9.0e+172,
        1.0e+173,  2.0e+173,  3.0e+173,  4.0e+173,  5.0e+173,  6.0e+173,  7.0e+173,  8.0e+173,  9.0e+173,
        1.0e+174,  2.0e+174,  3.0e+174,  4.0e+174,  5.0e+174,  6.0e+174,  7.0e+174,  8.0e+174,  9.0e+174,
        1.0e+175,  2.0e+175,  3.0e+175,  4.0e+175,  5.0e+175,  6.0e+175,  7.0e+175,  8.0e+175,  9.0e+175,
        1.0e+176,  2.0e+176,  3.0e+176,  4.0e+176,  5.0e+176,  6.0e+176,  7.0e+176,  8.0e+176,  9.0e+176,
        1.0e+177,  2.0e+177,  3.0e+177,  4.0e+177,  5.0e+177,  6.0e+177,  7.0e+177,  8.0e+177,  9.0e+177,
        1.0e+178,  2.0e+178,  3.0e+178,  4.0e+178,  5.0e+178,  6.0e+178,  7.0e+178,  8.0e+178,  9.0e+178,
        1.0e+179,  2.0e+179,  3.0e+179,  4.0e+179,  5.0e+179,  6.0e+179,  7.0e+179,  8.0e+179,  9.0e+179,
        1.0e+180,  2.0e+180,  3.0e+180,  4.0e+180,  5.0e+180,  6.0e+180,  7.0e+180,  8.0e+180,  9.0e+180,
        1.0e+181,  2.0e+181,  3.0e+181,  4.0e+181,  5.0e+181,  6.0e+181,  7.0e+181,  8.0e+181,  9.0e+181,
        1.0e+182,  2.0e+182,  3.0e+182,  4.0e+182,  5.0e+182,  6.0e+182,  7.0e+182,  8.0e+182,  9.0e+182,
        1.0e+183,  2.0e+183,  3.0e+183,  4.0e+183,  5.0e+183,  6.0e+183,  7.0e+183,  8.0e+183,  9.0e+183,
        1.0e+184,  2.0e+184,  3.0e+184,  4.0e+184,  5.0e+184,  6.0e+184,  7.0e+184,  8.0e+184,  9.0e+184,
        1.0e+185,  2.0e+185,  3.0e+185,  4.0e+185,  5.0e+185,  6.0e+185,  7.0e+185,  8.0e+185,  9.0e+185,
        1.0e+186,  2.0e+186,  3.0e+186,  4.0e+186,  5.0e+186,  6.0e+186,  7.0e+186,  8.0e+186,  9.0e+186,
        1.0e+187,  2.0e+187,  3.0e+187,  4.0e+187,  5.0e+187,  6.0e+187,  7.0e+187,  8.0e+187,  9.0e+187,
        1.0e+188,  2.0e+188,  3.0e+188,  4.0e+188,  5.0e+188,  6.0e+188,  7.0e+188,  8.0e+188,  9.0e+188,
        1.0e+189,  2.0e+189,  3.0e+189,  4.0e+189,  5.0e+189,  6.0e+189,  7.0e+189,  8.0e+189,  9.0e+189,
        1.0e+190,  2.0e+190,  3.0e+190,  4.0e+190,  5.0e+190,  6.0e+190,  7.0e+190,  8.0e+190,  9.0e+190,
        1.0e+191,  2.0e+191,  3.0e+191,  4.0e+191,  5.0e+191,  6.0e+191,  7.0e+191,  8.0e+191,  9.0e+191,
        1.0e+192,  2.0e+192,  3.0e+192,  4.0e+192,  5.0e+192,  6.0e+192,  7.0e+192,  8.0e+192,  9.0e+192,
        1.0e+193,  2.0e+193,  3.0e+193,  4.0e+193,  5.0e+193,  6.0e+193,  7.0e+193,  8.0e+193,  9.0e+193,
        1.0e+194,  2.0e+194,  3.0e+194,  4.0e+194,  5.0e+194,  6.0e+194,  7.0e+194,  8.0e+194,  9.0e+194,
        1.0e+195,  2.0e+195,  3.0e+195,  4.0e+195,  5.0e+195,  6.0e+195,  7.0e+195,  8.0e+195,  9.0e+195,
        1.0e+196,  2.0e+196,  3.0e+196,  4.0e+196,  5.0e+196,  6.0e+196,  7.0e+196,  8.0e+196,  9.0e+196,
        1.0e+197,  2.0e+197,  3.0e+197,  4.0e+197,  5.0e+197,  6.0e+197,  7.0e+197,  8.0e+197,  9.0e+197,
        1.0e+198,  2.0e+198,  3.0e+198,  4.0e+198,  5.0e+198,  6.0e+198,  7.0e+198,  8.0e+198,  9.0e+198,
        1.0e+199,  2.0e+199,  3.0e+199,  4.0e+199,  5.0e+199,  6.0e+199,  7.0e+199,  8.0e+199,  9.0e+199,
        1.0e+200,  2.0e+200,  3.0e+200,  4.0e+200,  5.0e+200,  6.0e+200,  7.0e+200,  8.0e+200,  9.0e+200,
        1.0e+201,  2.0e+201,  3.0e+201,  4.0e+201,  5.0e+201,  6.0e+201,  7.0e+201,  8.0e+201,  9.0e+201,
        1.0e+202,  2.0e+202,  3.0e+202,  4.0e+202,  5.0e+202,  6.0e+202,  7.0e+202,  8.0e+202,  9.0e+202,
        1.0e+203,  2.0e+203,  3.0e+203,  4.0e+203,  5.0e+203,  6.0e+203,  7.0e+203,  8.0e+203,  9.0e+203,
        1.0e+204,  2.0e+204,  3.0e+204,  4.0e+204,  5.0e+204,  6.0e+204,  7.0e+204,  8.0e+204,  9.0e+204,
        1.0e+205,  2.0e+205,  3.0e+205,  4.0e+205,  5.0e+205,  6.0e+205,  7.0e+205,  8.0e+205,  9.0e+205,
        1.0e+206,  2.0e+206,  3.0e+206,  4.0e+206,  5.0e+206,  6.0e+206,  7.0e+206,  8.0e+206,  9.0e+206,
        1.0e+207,  2.0e+207,  3.0e+207,  4.0e+207,  5.0e+207,  6.0e+207,  7.0e+207,  8.0e+207,  9.0e+207,
        1.0e+208,  2.0e+208,  3.0e+208,  4.0e+208,  5.0e+208,  6.0e+208,  7.0e+208,  8.0e+208,  9.0e+208,
        1.0e+209,  2.0e+209,  3.0e+209,  4.0e+209,  5.0e+209,  6.0e+209,  7.0e+209,  8.0e+209,  9.0e+209,
        1.0e+210,  2.0e+210,  3.0e+210,  4.0e+210,  5.0e+210,  6.0e+210,  7.0e+210,  8.0e+210,  9.0e+210,
        1.0e+211,  2.0e+211,  3.0e+211,  4.0e+211,  5.0e+211,  6.0e+211,  7.0e+211,  8.0e+211,  9.0e+211,
        1.0e+212,  2.0e+212,  3.0e+212,  4.0e+212,  5.0e+212,  6.0e+212,  7.0e+212,  8.0e+212,  9.0e+212,
        1.0e+213,  2.0e+213,  3.0e+213,  4.0e+213,  5.0e+213,  6.0e+213,  7.0e+213,  8.0e+213,  9.0e+213,
        1.0e+214,  2.0e+214,  3.0e+214,  4.0e+214,  5.0e+214,  6.0e+214,  7.0e+214,  8.0e+214,  9.0e+214,
        1.0e+215,  2.0e+215,  3.0e+215,  4.0e+215,  5.0e+215,  6.0e+215,  7.0e+215,  8.0e+215,  9.0e+215,
        1.0e+216,  2.0e+216,  3.0e+216,  4.0e+216,  5.0e+216,  6.0e+216,  7.0e+216,  8.0e+216,  9.0e+216,
        1.0e+217,  2.0e+217,  3.0e+217,  4.0e+217,  5.0e+217,  6.0e+217,  7.0e+217,  8.0e+217,  9.0e+217,
        1.0e+218,  2.0e+218,  3.0e+218,  4.0e+218,  5.0e+218,  6.0e+218,  7.0e+218,  8.0e+218,  9.0e+218,
        1.0e+219,  2.0e+219,  3.0e+219,  4.0e+219,  5.0e+219,  6.0e+219,  7.0e+219,  8.0e+219,  9.0e+219,
        1.0e+220,  2.0e+220,  3.0e+220,  4.0e+220,  5.0e+220,  6.0e+220,  7.0e+220,  8.0e+220,  9.0e+220,
        1.0e+221,  2.0e+221,  3.0e+221,  4.0e+221,  5.0e+221,  6.0e+221,  7.0e+221,  8.0e+221,  9.0e+221,
        1.0e+222,  2.0e+222,  3.0e+222,  4.0e+222,  5.0e+222,  6.0e+222,  7.0e+222,  8.0e+222,  9.0e+222,
        1.0e+223,  2.0e+223,  3.0e+223,  4.0e+223,  5.0e+223,  6.0e+223,  7.0e+223,  8.0e+223,  9.0e+223,
        1.0e+224,  2.0e+224,  3.0e+224,  4.0e+224,  5.0e+224,  6.0e+224,  7.0e+224,  8.0e+224,  9.0e+224,
        1.0e+225,  2.0e+225,  3.0e+225,  4.0e+225,  5.0e+225,  6.0e+225,  7.0e+225,  8.0e+225,  9.0e+225,
        1.0e+226,  2.0e+226,  3.0e+226,  4.0e+226,  5.0e+226,  6.0e+226,  7.0e+226,  8.0e+226,  9.0e+226,
        1.0e+227,  2.0e+227,  3.0e+227,  4.0e+227,  5.0e+227,  6.0e+227,  7.0e+227,  8.0e+227,  9.0e+227,
        1.0e+228,  2.0e+228,  3.0e+228,  4.0e+228,  5.0e+228,  6.0e+228,  7.0e+228,  8.0e+228,  9.0e+228,
        1.0e+229,  2.0e+229,  3.0e+229,  4.0e+229,  5.0e+229,  6.0e+229,  7.0e+229,  8.0e+229,  9.0e+229,
        1.0e+230,  2.0e+230,  3.0e+230,  4.0e+230,  5.0e+230,  6.0e+230,  7.0e+230,  8.0e+230,  9.0e+230,
        1.0e+231,  2.0e+231,  3.0e+231,  4.0e+231,  5.0e+231,  6.0e+231,  7.0e+231,  8.0e+231,  9.0e+231,
        1.0e+232,  2.0e+232,  3.0e+232,  4.0e+232,  5.0e+232,  6.0e+232,  7.0e+232,  8.0e+232,  9.0e+232,
        1.0e+233,  2.0e+233,  3.0e+233,  4.0e+233,  5.0e+233,  6.0e+233,  7.0e+233,  8.0e+233,  9.0e+233,
        1.0e+234,  2.0e+234,  3.0e+234,  4.0e+234,  5.0e+234,  6.0e+234,  7.0e+234,  8.0e+234,  9.0e+234,
        1.0e+235,  2.0e+235,  3.0e+235,  4.0e+235,  5.0e+235,  6.0e+235,  7.0e+235,  8.0e+235,  9.0e+235,
        1.0e+236,  2.0e+236,  3.0e+236,  4.0e+236,  5.0e+236,  6.0e+236,  7.0e+236,  8.0e+236,  9.0e+236,
        1.0e+237,  2.0e+237,  3.0e+237,  4.0e+237,  5.0e+237,  6.0e+237,  7.0e+237,  8.0e+237,  9.0e+237,
        1.0e+238,  2.0e+238,  3.0e+238,  4.0e+238,  5.0e+238,  6.0e+238,  7.0e+238,  8.0e+238,  9.0e+238,
        1.0e+239,  2.0e+239,  3.0e+239,  4.0e+239,  5.0e+239,  6.0e+239,  7.0e+239,  8.0e+239,  9.0e+239,
        1.0e+240,  2.0e+240,  3.0e+240,  4.0e+240,  5.0e+240,  6.0e+240,  7.0e+240,  8.0e+240,  9.0e+240,
        1.0e+241,  2.0e+241,  3.0e+241,  4.0e+241,  5.0e+241,  6.0e+241,  7.0e+241,  8.0e+241,  9.0e+241,
        1.0e+242,  2.0e+242,  3.0e+242,  4.0e+242,  5.0e+242,  6.0e+242,  7.0e+242,  8.0e+242,  9.0e+242,
        1.0e+243,  2.0e+243,  3.0e+243,  4.0e+243,  5.0e+243,  6.0e+243,  7.0e+243,  8.0e+243,  9.0e+243,
        1.0e+244,  2.0e+244,  3.0e+244,  4.0e+244,  5.0e+244,  6.0e+244,  7.0e+244,  8.0e+244,  9.0e+244,
        1.0e+245,  2.0e+245,  3.0e+245,  4.0e+245,  5.0e+245,  6.0e+245,  7.0e+245,  8.0e+245,  9.0e+245,
        1.0e+246,  2.0e+246,  3.0e+246,  4.0e+246,  5.0e+246,  6.0e+246,  7.0e+246,  8.0e+246,  9.0e+246,
        1.0e+247,  2.0e+247,  3.0e+247,  4.0e+247,  5.0e+247,  6.0e+247,  7.0e+247,  8.0e+247,  9.0e+247,
        1.0e+248,  2.0e+248,  3.0e+248,  4.0e+248,  5.0e+248,  6.0e+248,  7.0e+248,  8.0e+248,  9.0e+248,
        1.0e+249,  2.0e+249,  3.0e+249,  4.0e+249,  5.0e+249,  6.0e+249,  7.0e+249,  8.0e+249,  9.0e+249,
        1.0e+250,  2.0e+250,  3.0e+250,  4.0e+250,  5.0e+250,  6.0e+250,  7.0e+250,  8.0e+250,  9.0e+250,
        1.0e+251,  2.0e+251,  3.0e+251,  4.0e+251,  5.0e+251,  6.0e+251,  7.0e+251,  8.0e+251,  9.0e+251,
        1.0e+252,  2.0e+252,  3.0e+252,  4.0e+252,  5.0e+252,  6.0e+252,  7.0e+252,  8.0e+252,  9.0e+252,
        1.0e+253,  2.0e+253,  3.0e+253,  4.0e+253,  5.0e+253,  6.0e+253,  7.0e+253,  8.0e+253,  9.0e+253,
        1.0e+254,  2.0e+254,  3.0e+254,  4.0e+254,  5.0e+254,  6.0e+254,  7.0e+254,  8.0e+254,  9.0e+254,
        1.0e+255,  2.0e+255,  3.0e+255,  4.0e+255,  5.0e+255,  6.0e+255,  7.0e+255,  8.0e+255,  9.0e+255,
        1.0e+256,  2.0e+256,  3.0e+256,  4.0e+256,  5.0e+256,  6.0e+256,  7.0e+256,  8.0e+256,  9.0e+256,
        1.0e+257,  2.0e+257,  3.0e+257,  4.0e+257,  5.0e+257,  6.0e+257,  7.0e+257,  8.0e+257,  9.0e+257,
        1.0e+258,  2.0e+258,  3.0e+258,  4.0e+258,  5.0e+258,  6.0e+258,  7.0e+258,  8.0e+258,  9.0e+258,
        1.0e+259,  2.0e+259,  3.0e+259,  4.0e+259,  5.0e+259,  6.0e+259,  7.0e+259,  8.0e+259,  9.0e+259,
        1.0e+260,  2.0e+260,  3.0e+260,  4.0e+260,  5.0e+260,  6.0e+260,  7.0e+260,  8.0e+260,  9.0e+260,
        1.0e+261,  2.0e+261,  3.0e+261,  4.0e+261,  5.0e+261,  6.0e+261,  7.0e+261,  8.0e+261,  9.0e+261,
        1.0e+262,  2.0e+262,  3.0e+262,  4.0e+262,  5.0e+262,  6.0e+262,  7.0e+262,  8.0e+262,  9.0e+262,
        1.0e+263,  2.0e+263,  3.0e+263,  4.0e+263,  5.0e+263,  6.0e+263,  7.0e+263,  8.0e+263,  9.0e+263,
        1.0e+264,  2.0e+264,  3.0e+264,  4.0e+264,  5.0e+264,  6.0e+264,  7.0e+264,  8.0e+264,  9.0e+264,
        1.0e+265,  2.0e+265,  3.0e+265,  4.0e+265,  5.0e+265,  6.0e+265,  7.0e+265,  8.0e+265,  9.0e+265,
        1.0e+266,  2.0e+266,  3.0e+266,  4.0e+266,  5.0e+266,  6.0e+266,  7.0e+266,  8.0e+266,  9.0e+266,
        1.0e+267,  2.0e+267,  3.0e+267,  4.0e+267,  5.0e+267,  6.0e+267,  7.0e+267,  8.0e+267,  9.0e+267,
        1.0e+268,  2.0e+268,  3.0e+268,  4.0e+268,  5.0e+268,  6.0e+268,  7.0e+268,  8.0e+268,  9.0e+268,
        1.0e+269,  2.0e+269,  3.0e+269,  4.0e+269,  5.0e+269,  6.0e+269,  7.0e+269,  8.0e+269,  9.0e+269,
        1.0e+270,  2.0e+270,  3.0e+270,  4.0e+270,  5.0e+270,  6.0e+270,  7.0e+270,  8.0e+270,  9.0e+270,
        1.0e+271,  2.0e+271,  3.0e+271,  4.0e+271,  5.0e+271,  6.0e+271,  7.0e+271,  8.0e+271,  9.0e+271,
        1.0e+272,  2.0e+272,  3.0e+272,  4.0e+272,  5.0e+272,  6.0e+272,  7.0e+272,  8.0e+272,  9.0e+272,
        1.0e+273,  2.0e+273,  3.0e+273,  4.0e+273,  5.0e+273,  6.0e+273,  7.0e+273,  8.0e+273,  9.0e+273,
        1.0e+274,  2.0e+274,  3.0e+274,  4.0e+274,  5.0e+274,  6.0e+274,  7.0e+274,  8.0e+274,  9.0e+274,
        1.0e+275,  2.0e+275,  3.0e+275,  4.0e+275,  5.0e+275,  6.0e+275,  7.0e+275,  8.0e+275,  9.0e+275,
        1.0e+276,  2.0e+276,  3.0e+276,  4.0e+276,  5.0e+276,  6.0e+276,  7.0e+276,  8.0e+276,  9.0e+276,
        1.0e+277,  2.0e+277,  3.0e+277,  4.0e+277,  5.0e+277,  6.0e+277,  7.0e+277,  8.0e+277,  9.0e+277,
        1.0e+278,  2.0e+278,  3.0e+278,  4.0e+278,  5.0e+278,  6.0e+278,  7.0e+278,  8.0e+278,  9.0e+278,
        1.0e+279,  2.0e+279,  3.0e+279,  4.0e+279,  5.0e+279,  6.0e+279,  7.0e+279,  8.0e+279,  9.0e+279,
        1.0e+280,  2.0e+280,  3.0e+280,  4.0e+280,  5.0e+280,  6.0e+280,  7.0e+280,  8.0e+280,  9.0e+280,
        1.0e+281,  2.0e+281,  3.0e+281,  4.0e+281,  5.0e+281,  6.0e+281,  7.0e+281,  8.0e+281,  9.0e+281,
        1.0e+282,  2.0e+282,  3.0e+282,  4.0e+282,  5.0e+282,  6.0e+282,  7.0e+282,  8.0e+282,  9.0e+282,
        1.0e+283,  2.0e+283,  3.0e+283,  4.0e+283,  5.0e+283,  6.0e+283,  7.0e+283,  8.0e+283,  9.0e+283,
        1.0e+284,  2.0e+284,  3.0e+284,  4.0e+284,  5.0e+284,  6.0e+284,  7.0e+284,  8.0e+284,  9.0e+284,
        1.0e+285,  2.0e+285,  3.0e+285,  4.0e+285,  5.0e+285,  6.0e+285,  7.0e+285,  8.0e+285,  9.0e+285,
        1.0e+286,  2.0e+286,  3.0e+286,  4.0e+286,  5.0e+286,  6.0e+286,  7.0e+286,  8.0e+286,  9.0e+286,
        1.0e+287,  2.0e+287,  3.0e+287,  4.0e+287,  5.0e+287,  6.0e+287,  7.0e+287,  8.0e+287,  9.0e+287,
        1.0e+288,  2.0e+288,  3.0e+288,  4.0e+288,  5.0e+288,  6.0e+288,  7.0e+288,  8.0e+288,  9.0e+288,
        1.0e+289,  2.0e+289,  3.0e+289,  4.0e+289,  5.0e+289,  6.0e+289,  7.0e+289,  8.0e+289,  9.0e+289,
        1.0e+290,  2.0e+290,  3.0e+290,  4.0e+290,  5.0e+290,  6.0e+290,  7.0e+290,  8.0e+290,  9.0e+290,
        1.0e+291,  2.0e+291,  3.0e+291,  4.0e+291,  5.0e+291,  6.0e+291,  7.0e+291,  8.0e+291,  9.0e+291,
        1.0e+292,  2.0e+292,  3.0e+292,  4.0e+292,  5.0e+292,  6.0e+292,  7.0e+292,  8.0e+292,  9.0e+292,
        1.0e+293,  2.0e+293,  3.0e+293,  4.0e+293,  5.0e+293,  6.0e+293,  7.0e+293,  8.0e+293,  9.0e+293,
        1.0e+294,  2.0e+294,  3.0e+294,  4.0e+294,  5.0e+294,  6.0e+294,  7.0e+294,  8.0e+294,  9.0e+294,
        1.0e+295,  2.0e+295,  3.0e+295,  4.0e+295,  5.0e+295,  6.0e+295,  7.0e+295,  8.0e+295,  9.0e+295,
        1.0e+296,  2.0e+296,  3.0e+296,  4.0e+296,  5.0e+296,  6.0e+296,  7.0e+296,  8.0e+296,  9.0e+296,
        1.0e+297,  2.0e+297,  3.0e+297,  4.0e+297,  5.0e+297,  6.0e+297,  7.0e+297,  8.0e+297,  9.0e+297,
        1.0e+298,  2.0e+298,  3.0e+298,  4.0e+298,  5.0e+298,  6.0e+298,  7.0e+298,  8.0e+298,  9.0e+298,
        1.0e+299,  2.0e+299,  3.0e+299,  4.0e+299,  5.0e+299,  6.0e+299,  7.0e+299,  8.0e+299,  9.0e+299,
        1.0e+300,  2.0e+300,  3.0e+300,  4.0e+300,  5.0e+300,  6.0e+300,  7.0e+300,  8.0e+300,  9.0e+300,
        1.0e+301,  2.0e+301,  3.0e+301,  4.0e+301,  5.0e+301,  6.0e+301,  7.0e+301,  8.0e+301,  9.0e+301,
        1.0e+302,  2.0e+302,  3.0e+302,  4.0e+302,  5.0e+302,  6.0e+302,  7.0e+302,  8.0e+302,  9.0e+302,
        1.0e+303,  2.0e+303,  3.0e+303,  4.0e+303,  5.0e+303,  6.0e+303,  7.0e+303,  8.0e+303,  9.0e+303,
        1.0e+304,  2.0e+304,  3.0e+304,  4.0e+304,  5.0e+304,  6.0e+304,  7.0e+304,  8.0e+304,  9.0e+304,
        1.0e+305,  2.0e+305,  3.0e+305,  4.0e+305,  5.0e+305,  6.0e+305,  7.0e+305,  8.0e+305,  9.0e+305,
        1.0e+306,  2.0e+306,  3.0e+306,  4.0e+306,  5.0e+306,  6.0e+306,  7.0e+306,  8.0e+306,  9.0e+306,
        1.0e+307,  2.0e+307,  3.0e+307,  4.0e+307,  5.0e+307,  6.0e+307,  7.0e+307,  8.0e+307,  9.0e+307,
        1.0e+308,  HUGE_VAL,  HUGE_VAL,  HUGE_VAL,  HUGE_VAL,  HUGE_VAL,  HUGE_VAL,  HUGE_VAL,  HUGE_VAL,
      };

    S_real_parts do_decompose_real_decimal(const G_real& value)
      {
        // Break the number down into the significand and exponent parts.
        S_real_parts p;
        p.reg = std::fabs(value);
        // Calculate the exponent.
        auto bp = std::begin(s_decimals);
        auto mi = static_cast<int>(std::upper_bound(bp, std::end(s_decimals), p.reg) - bp - 1);
        // Note that the first two elements are zeroes.
        if(mi < 2) {
          p.exp = -1;
        } else {
          p.exp = mi / 9 - 324;
        }
        return p;
      }

    void do_extract_digit_real_decimal(G_string& text, S_real_parts& p)
      {
        // Shift a digit out.
        int off;
        if(p.exp < -324) {
          p.reg = 0;
          off = 0;
        } else {
          auto bp = std::begin(s_decimals) + (p.exp + 324) * 9;
          auto mi = static_cast<int>(std::upper_bound(bp, bp + 9, p.reg) - bp - 1);
          if(mi >= 0) {
            p.reg -= bp[mi];
          }
          off = mi + 1;
        }
        p.exp--;
        // Locate the digit in uppercase.
        off *= 2;
        off += 1;
        // Write this digit.
        text.push_back(s_xdigits[off]);
      }

    G_string do_format_real_decimal(const G_real& value, std::uint8_t rbase)
      {
        G_string text;
        if(!do_preformat_real(text, value, rbase)) {
          return text;
        }
        // Get the exponent.
        auto p = do_decompose_real_decimal(value);
        // If the value is less than one, write it after a `0.` prefix and prepend zeroes as needed.
        if(p.exp <= -1) {
          text.append("0.");
          text.append(static_cast<std::size_t>(-1 - p.exp), '0');
        }
        // Write all significant figures.
        for(int i = 0; i < 17; ++i) {
          do_extract_digit_real_decimal(text, p);
          // If it was the last one of the integral part, append a decimal point.
          if(p.exp == -1) {
            text.push_back('.');
          }
        }
        // Fill zeroes until the end of the integral part has been reached.
        if(p.exp > -1) {
          text.append(static_cast<std::size_t>(p.exp - -1), '0');
          text.push_back('.');
        }
        // The string will always contain a decimal point.
        // Remove all trailing zeroes.
        while(text.back() == '0') {
          text.pop_back();
        }
        // Append a zero digit if there is no fractional part.
        if(text.back() == '.') {
          text.push_back('0');
        }
        return text;
      }

    G_string do_format_real_mixed_p_scientific(const G_real& value, std::uint8_t rbase, std::uint8_t pbase)
      {
        G_string text;
        if(!do_preformat_real(text, value, rbase)) {
          return text;
        }
        // Rewrite the value in scientific notation.
        auto p = do_decompose_real_exact(value, pbase);
        auto exp = p.exp;
        // Rebase from `pbase` to `10`.
        p.exp = 0;
        // Write the first significant figure that precedes the decimal point.
        do_extract_digit_real_decimal(text, p);
        // Write the decimal point.
        text.push_back('.');
        // Write all significant figures that follow the decimal point.
        for(int i = 0; i < 17; ++i) {
          do_extract_digit_real_decimal(text, p);
        }
        // The string will always contain a decimal point.
        // Remove all trailing zeroes.
        while(text.back() == '0') {
          text.pop_back();
        }
        // Append a zero digit if there is no fractional part.
        if(text.back() == '.') {
          text.push_back('0');
        }
        // Write the exponent.
        do_format_exponent(text, exp, pbase);
        return text;
      }

    G_string do_format_real_decimal_scientific(const G_real& value, std::uint8_t rbase, std::uint8_t pbase)
      {
        G_string text;
        if(!do_preformat_real(text, value, rbase)) {
          return text;
        }
        // Rewrite the value in scientific notation.
        auto p = do_decompose_real_decimal(value);
        auto exp = p.exp;
        // Write the first significant figure that precedes the decimal point.
        do_extract_digit_real_decimal(text, p);
        // Write the decimal point.
        text.push_back('.');
        // Write all significant figures that follow the decimal point.
        for(int i = 0; i < 17; ++i) {
          do_extract_digit_real_decimal(text, p);
        }
        // The string will always contain a decimal point.
        // Remove all trailing zeroes.
        while(text.back() == '0') {
          text.pop_back();
        }
        // Append a zero digit if there is no fractional part.
        if(text.back() == '.') {
          text.push_back('0');
        }
        // Write the exponent.
        do_format_exponent(text, exp, pbase);
        return text;
      }

    }

G_string std_numeric_format(const G_real& value, const Opt<G_integer>& base, const Opt<G_integer>& ebase)
  {
    switch(base.value_or(10)) {
    case  2:
      {
        if(!ebase) {
          return do_format_real_exact(value,  2);
        }
        if(*ebase ==  2) {
          return do_format_real_exact_scientific(value,  2,  2);
        }
        ASTERIA_THROW_RUNTIME_ERROR("The base of the exponent of a number in binary must be `2` (got `", *ebase, "`).");
      }
    case 16:
      {
        if(!ebase) {
          return do_format_real_exact(value, 16);
        }
        if(*ebase ==  2) {
          return do_format_real_exact_scientific(value, 16,  2);
        }
        ASTERIA_THROW_RUNTIME_ERROR("The base of the exponent of a number in hexadecimal must be `2` (got `", *ebase, "`).");
      }
    case 10:
      {
        if(!ebase) {
          return do_format_real_decimal(value, 10);
        }
        if(*ebase ==  2) {
          return do_format_real_mixed_p_scientific(value, 10,  2);
        }
        if(*ebase == 10) {
          return do_format_real_decimal_scientific(value, 10, 10);
        }
        ASTERIA_THROW_RUNTIME_ERROR("The base of the exponent of a number in decimal must be either `2` or `10` (got `", *ebase, "`).");
      }
    default:
      ASTERIA_THROW_RUNTIME_ERROR("The base of a number must be either `2` or `10` or `16` (got `", *base, "`).");
    }
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

    inline void do_raise_real(double& value, std::uint8_t base, std::int64_t exp) noexcept
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
    std::int64_t icnt = 0;  // number of integral digits (always non-negative)
    std::uint8_t pbase = 0;  // the base of the exponent.
    bool pneg = false;  // is the exponent negative?
    std::int64_t pexp = 0;  // `pbase`'d exponent
    std::int64_t pcnt = 0;  // number of exponent digits (always non-negative)
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
      switch(text[tpos + 1]) {
      case 'b':
      case 'B':
        tpos += 2;
        rbase =  2;
        break;
      case 'x':
      case 'X':
        tpos += 2;
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
      icnt++;
      // Is the next character a digit separator?
      if(text[tpos] == '`') {
        tpos++;
      }
    }
    // There shall be at least one digit.
    if(icnt == 0) {
      return rocket::nullopt;
    }
    // Check for the exponent part.
    switch(text[tpos]) {
    case 'p':
    case 'P':
      tpos++;
      pbase =  2;
      break;
    case 'e':
    case 'E':
      tpos++;
      pbase = 10;
      break;
    }
    if(pbase != 0) {
      // Get the sign of the exponent if any.
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
        pcnt++;
        // Is the next character a digit separator?
        if(text[tpos] == '`') {
          tpos++;
        }
      }
      // There shall be at least one digit.
      if(pcnt == 0) {
        return rocket::nullopt;
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
    bool pneg = false;  // is the exponent negative?
    std::int64_t pexp = 0;  // `pbase`'d exponent
    std::int64_t pcnt = 0;  // number of exponent digits (always non-negative)
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
      switch(text[tpos + 1]) {
      case 'b':
      case 'B':
        tpos += 2;
        rbase = 2;
        break;
      case 'x':
      case 'X':
        tpos += 2;
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
      {
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
      }
    case 'n':
    case 'N':
      {
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
      }
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
    // There shall be at least one digit.
    if(icnt == 0) {
      return rocket::nullopt;
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
      // There shall be at least one digit.
      if(fcnt == 0) {
        return rocket::nullopt;
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
        pcnt++;
        // Is the next character a digit separator?
        if(text[tpos] == '`') {
          tpos++;
        }
      }
      // There shall be at least one digit.
      if(pcnt == 0) {
        return rocket::nullopt;
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
      do_raise_real(value, rbase, tcnt);
      do_raise_real(value, pbase, pexp);
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
            // Parse arguments.
            G_integer ix;
            G_integer iy;
            if(reader.start().g(ix).g(iy).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_adds(ix, iy) };
              return rocket::move(xref);
            }
            G_real fx;
            G_real fy;
            if(reader.start().g(fx).g(fy).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_adds(fx, fy) };
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
            // Parse arguments.
            G_integer ix;
            G_integer iy;
            if(reader.start().g(ix).g(iy).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subs(ix, iy) };
              return rocket::move(xref);
            }
            G_real fx;
            G_real fy;
            if(reader.start().g(fx).g(fy).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subs(fx, fy) };
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
            // Parse arguments.
            G_integer ix;
            G_integer iy;
            if(reader.start().g(ix).g(iy).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_muls(ix, iy) };
              return rocket::move(xref);
            }
            G_real fx;
            G_real fy;
            if(reader.start().g(fx).g(fy).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_muls(fx, fy) };
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
            "    No plus sign precedes the significant figures. If `base` is\n"
            "    absent, `10` is assumed. If `ebase` is specified, an exponent\n"
            "    is appended to the significand as follows: If `value` is of\n"
            "    type `integer`, the significand is kept as short as possible;\n"
            "    otherwise (when `value` is of type `real`), it is written in\n"
            "    scientific notation, whose significand always contains a\n"
            "    decimal point. In both cases, the exponent comprises a plus or\n"
            "    minus sign and at least two digits. If `ebase` is absent, no\n"
            "    exponent appears. The result is exact as long as `base` is a\n"
            "    power of two.\n"
            "  \n"
            "  * Returns a `string` converted from `value`.\n"
            "  \n"
            "  * Throws an exception if `base` is neither `2` nor `10` nor `16`,\n"
            "    or if `ebase` is neither `2` nor `10`, or if `base` is not `10`\n"
            "    but `ebase` is `10`.\n"
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
            // Parse arguments.
            G_integer ivalue;
            Opt<G_integer> base;
            Opt<G_integer> ebase;
            if(reader.start().g(ivalue).g(base).g(ebase).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_format(ivalue, base, ebase) };
              return rocket::move(xref);
            }
            G_real fvalue;
            if(reader.start().g(fvalue).g(base).g(ebase).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_format(fvalue, base, ebase) };
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
