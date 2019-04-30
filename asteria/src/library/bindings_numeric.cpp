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

    inline const G_integer& do_verify_bounds(const G_integer& lower, const G_integer& upper)
      {
        if(!(lower <= upper)) {
          ASTERIA_THROW_RUNTIME_ERROR("The `lower` bound must be less than or equal to the `upper` bound (got `", lower, "` and `", upper, "`).");
        }
        return upper;
      }
    inline const G_real& do_verify_bounds(const G_real& lower, const G_real& upper)
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

    namespace {

    double do_random_ratio(const Global_Context& global) noexcept
      {
        // sqword <= [0,INT64_MAX]
        std::int64_t sqword = global.get_random_uint32();
        sqword <<= 31;
        sqword ^= global.get_random_uint32();
        // return <= [0,1)
        return static_cast<double>(sqword) / 0x1p63;
      }

    }

G_integer std_numeric_random(const Global_Context& global, const G_integer& upper)
  {
    if(!(0 < upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `upper` bound must be greater than zero (got `", upper, "`).");
    }
    auto res = do_random_ratio(global);
    res *= static_cast<double>(upper);
    return static_cast<std::int64_t>(res);
  }

G_real std_numeric_random(const Global_Context& global, const Opt<G_real>& upper)
  {
    if(upper && !std::isless(0, *upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `upper` bound must be greater than zero (got `", *upper, "`).");
    }
    auto res = do_random_ratio(global);
    if(upper) {
      res *= *upper;
    }
    return res;
  }

G_integer std_numeric_random(const Global_Context& global, const G_integer& lower, const G_integer& upper)
  {
    if(!(lower < upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` bound must be less than the `upper` bound (got `", lower, "` and `", upper, "`).");
    }
    auto res = do_random_ratio(global);
    res *= static_cast<double>(upper - lower);
    return lower + static_cast<std::int64_t>(res);
  }

G_real std_numeric_random(const Global_Context& global, const G_real& lower, const G_real& upper)
  {
    if(!std::isless(lower, upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` bound must be less than the `upper` bound (got `", lower, "` and `", upper, "`).");
    }
    auto res = do_random_ratio(global);
    res *= upper - lower;
    return lower + res;
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

    constexpr char s_digits[] = "00112233445566778899AaBbCcDdEeFf";

    inline std::uint8_t do_verify_base(const Opt<G_integer>& base)
      {
        // The default base is `10`.
        std::uint8_t value = 10;
        if(base) {
          // If the user has provided a custom base, ensure it is acceptable.
          if((*base != 2) && (*base != 10) && (*base != 16)) {
            ASTERIA_THROW_RUNTIME_ERROR("`base` must be either `2`, `10` or `16` (got `", *base, "`).");
          }
          value = static_cast<std::uint8_t>(*base);
        }
        return value;
      }

    inline std::uint8_t do_verify_exp_base(const Opt<G_integer>& exp_base)
      {
        // The default base is `10`.
        std::uint8_t value = 10;
        if(exp_base) {
          // If the user has provided a custom base, ensure it is acceptable.
          if((*exp_base != 2) && (*exp_base != 10)) {
            ASTERIA_THROW_RUNTIME_ERROR("`exp_base` must be either `2` or `10` (got `", *exp_base, "`).");
          }
          value = static_cast<std::uint8_t>(*exp_base);
        }
        return value;
      }

    inline bool do_handle_special_values(G_string& text, const G_real& value)
      {
        const char* sptr;
        // Get FP class.
        switch(std::fpclassify(value)) {
        case FP_INFINITE:
          sptr = "-infinity";
          break;
        case FP_NAN:
          sptr = "-nan";
          break;
        case FP_ZERO:
          sptr = "-0";
          break;
        default:
          return false;
        }
        // Check whether to remove the leading minus sign.
        sptr += !std::signbit(value);
        text = rocket::sref(sptr);
        return true;
      }

    inline std::uint8_t do_shift_digit(G_integer& reg, bool neg, std::uint8_t base)
      {
        auto dvalue = static_cast<int>(reg % base);
        reg /= base;
        // Return the absolute value of the digit.
        return static_cast<std::uint8_t>((dvalue ^ -neg) + neg);
      }
    inline std::uint8_t do_shift_digit(G_real& reg, bool neg, std::uint8_t base)
      {
        auto frac = std::modf(reg / base, &reg);
        auto dvalue = static_cast<std::int64_t>(frac * base + 0.5 - neg);
        // Return the absolute value of the digit.
        return static_cast<std::uint8_t>((dvalue ^ -neg) + neg);
      }

    template<typename XvalueT> void do_append_integer_reverse(G_string& text, int& count, const XvalueT& value, bool neg, std::uint8_t base)
      {
        auto reg = value;
        do {
          auto dvalue = do_shift_digit(reg, neg, base);
          text.push_back(s_digits[dvalue * 2]);
          count--;
        } while(reg != 0);
      }

    G_string& do_append_integer_prefixes(G_string& text, bool neg, std::uint8_t base)
      {
        // N.B. Characters are appended in reverse order.
        if(base == 2) {
          text.push_back('b');
          text.push_back('0');
        }
        if(base == 16) {
          text.push_back('x');
          text.push_back('0');
        }
        if(neg) {
          text.push_back('-');
        }
        return text;
      }

    inline std::uint8_t do_pop_digit(G_real& reg, bool neg, std::uint8_t base)
      {
        G_real intg;
        reg = std::modf(reg * base, &intg);
        auto dvalue = static_cast<std::int64_t>(intg);
        // Return the absolute value of the digit.
        return static_cast<std::uint8_t>((dvalue ^ -neg) + neg);
      }

    template<typename XvalueT> void do_append_fraction_normal(G_string& text, int& count, const XvalueT& value, bool neg, std::uint8_t base)
      {
        // Write at least one digit.
        auto reg = value;
        do {
          auto dvalue = do_pop_digit(reg, neg, base);
          text.push_back(s_digits[dvalue * 2]);
          count--;
        } while((reg != 0) && (count > 0));
      }

    G_string& do_append_exponent_prefixes(G_string& text, bool neg, std::uint8_t ebase)
      {
        // N.B. Characters are appended in reverse order.
        if(neg) {
          text.push_back('-');
        }
        if(ebase == 2) {
          text.push_back('p');
        }
        if(ebase == 10) {
          text.push_back('e');
        }
        return text;
      }

    G_string& do_reverse_suffix(G_string& text, std::size_t from)
      {
        // Get the suffix range.
        std::size_t b = from;
        std::size_t e = text.size();
        if(b >= e) {
          return text;
        }
        char* ptr = text.mut_data();
        while(e - b >= 2) {
          --e;
          std::swap(ptr[b], ptr[e]);
          ++b;
        }
        return text;
      }

    }

G_string std_numeric_format(const G_integer& value, const Opt<G_integer>& base)
  {
    bool rneg = value < 0;
    std::uint8_t rbase = do_verify_base(base);
    // Define the result string.
    G_string text;
    // The string will always be exact.
    int rcount = SCHAR_MAX;
    // The value itself is the integral part. There are no fractional or exponent parts.
    G_integer intg = value;
    // Write the integral part.
    do_append_integer_reverse(text, rcount, intg, rneg, rbase);
    do_append_integer_prefixes(text, rneg, rbase);
    do_reverse_suffix(text, 0);
    // Finish.
    return text;
  }

G_string std_numeric_format(const G_real& value, const Opt<G_integer>& base)
  {
    bool rneg = std::signbit(value);
    std::uint8_t rbase = do_verify_base(base);
    // Define the result string.
    // When `value` is special, the other arguments still have to be valid.
    G_string text;
    if(do_handle_special_values(text, value)) {
      return text;
    }
    // The string will be exact if and only if the base is a power of two.
    int rcount = (rbase % 2 == 0) ? SCHAR_MAX : static_cast<int>(std::ceil(53 / std::log2(rbase) - 0.001) + 1);
    // Break the number down into integral and fractional parts.
    G_real intg;
    G_real frac = std::modf(value, &intg);
    // Write the integral part.
    do_append_integer_reverse(text, rcount, intg, rneg, rbase);
    do_append_integer_prefixes(text, rneg, rbase);
    do_reverse_suffix(text, 0);
    // If the fractional part is not zero, append it after a decimal point.
    if((frac != 0) && (rcount > 0)) {
      text.push_back('.');
      do_append_fraction_normal(text, rcount, frac, rneg, rbase);
    }
    // Finish.
    return text;
  }

G_string std_numeric_format(const G_integer& value, const Opt<G_integer>& base, const G_integer& exp_base)
  {
    bool rneg = value < 0;
    std::uint8_t rbase = do_verify_base(base);
    std::uint8_t ebase = do_verify_exp_base(exp_base);
    // Define the result string.
    G_string text;
    // The string will always be exact.
    int rcount = SCHAR_MAX;
    int ecount = SCHAR_MAX;
    // The value itself is the integral part. There are no fractional or exponent parts.
    G_integer intg = value;
    // Calculate the exponent. In the case of integers it will never be negative.
    G_integer eint = 0;
    for(;;) {
      auto next = intg / ebase;
      if(intg % ebase != 0) {
        break;
      }
      intg = next;
      eint++;
    }
    bool eneg = eint < 0;
    // Write the integral part.
    do_append_integer_reverse(text, rcount, intg, rneg, rbase);
    do_append_integer_prefixes(text, rneg, rbase);
    do_reverse_suffix(text, 0);
    // Write the exponent part.
    auto expb = text.size();
    do_append_integer_reverse(text, ecount, eint, eneg, 10);
    do_append_exponent_prefixes(text, eneg, ebase);
    do_reverse_suffix(text, expb);
    // Finish.
    return text;
  }

G_string std_numeric_format(const G_real& value, const Opt<G_integer>& base, const G_integer& exp_base)
  {
    bool rneg = std::signbit(value);
    std::uint8_t rbase = do_verify_base(base);
    std::uint8_t ebase = do_verify_exp_base(exp_base);
    // Define the result string.
    // When `value` is special, the other arguments still have to be valid.
    G_string text;
    if(do_handle_special_values(text, value)) {
      return text;
    }
    // The string will be exact if and only if the base is a power of two.
    int rcount = (rbase % 2 == 0) ? SCHAR_MAX : static_cast<int>(std::ceil(53 / std::log2(rbase) - 0.001) + 1);
    int ecount = SCHAR_MAX;
    // Calculate the exponent.
    G_integer eint = 0;
    G_real epow;
    switch(ebase) {
    case 2:
      {
        auto elog = std::floor(std::logb(std::abs(value)));
        eint = static_cast<std::int64_t>(elog);
        epow = std::exp2(elog);
        break;
      }
    case 10:
      {
        auto elog = std::floor(std::log10(std::abs(value)));
        eint = static_cast<std::int64_t>(elog);
        epow = std::pow(10, elog);
        break;
      }
    default:
      ROCKET_ASSERT(false);
    }
    bool eneg = eint < 0;
    // Break the number down into integral and fractional parts.
    G_real intg;
    G_real frac = std::modf(value / epow, &intg);
    // Write the integral part.
    do_append_integer_reverse(text, rcount, intg, rneg, rbase);
    do_append_integer_prefixes(text, rneg, rbase);
    do_reverse_suffix(text, 0);
    // If the fractional part is not zero, append it after a decimal point.
    if((frac != 0) && (rcount > 0)) {
      text.push_back('.');
      do_append_fraction_normal(text, rcount, frac, rneg, rbase);
    }
    // Write the exponent part.
    auto expb = text.size();
    do_append_integer_reverse(text, ecount, eint, eneg, 10);
    do_append_exponent_prefixes(text, eneg, ebase);
    do_reverse_suffix(text, expb);
    // Finish.
    return text;
  }

G_integer std_numeric_parse_integer(const G_string& text)
  {
    return -1;
  }

G_real std_numeric_parse_real(const G_string& text, const Opt<G_boolean>& saturating)
  {
    return -1;
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
            "`std.numeric.random([upper])`\n"
            "  \n"
            "  * Generates a random `integer` or `real` that is less than\n"
            "    `upper`. If `upper` is absent, it has a default value of `1.0`\n"
            "    which is a `real`.\n"
            "  \n"
            "  * Returns a non-negative `integer` or `real` that is less than\n"
            "    `upper`. The return value is of type `integer` if `upper` is of\n"
            "    type `integer`; otherwise it is of type `real`.\n"
            "  \n"
            "  * Throws an exception if `upper` is negative or zero.\n"
            "\n"
            "`std.numeric.random(lower, upper)`\n"
            "  \n"
            "  * Generates a random `integer` or `real` that is not less than\n"
            "    `lower` but is less than `upper`. `lower` and `upper` shall be\n"
            "    of the same type.\n"
            "  \n"
            "  * Returns an `integer` or `real` that is not less than `lower`\n"
            "    but is less than `upper`. The return value is of type `integer`\n"
            "    if both arguments are of type `integer`; otherwise it is of\n"
            "    type `real`.\n"
            "  \n"
            "  * Throws an exception if `lower` is not less than `upper`.\n"
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
            G_integer iupper;
            if(reader.start().g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(global, iupper) };
              return rocket::move(xref);
            }
            Opt<G_real> kupper;
            if(reader.start().g(kupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(global, kupper) };
              return rocket::move(xref);
            }
            G_integer ilower;
            if(reader.start().g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(global, ilower, iupper) };
              return rocket::move(xref);
            }
            G_real fupper;
            G_real flower;
            if(reader.start().g(flower).g(fupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(global, flower, fupper) };
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
            "    function writes as many digits as possible for precision. If\n"
            "    `value` is non-negative, no plus sign appears. If `base` is not\n"
            "    specified, `10` is assumed. When `exo_base` is specified, if\n"
            "    `value` is of type `real`, the number is written in scientific\n"
            "    notation; otherwise (when it is of type `integer`), an exponent\n"
            "    part is still appended, however no decimal point shall occur in\n"
            "    the result.\n"
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
            "    * Base-2:   `[+-]?0b[01]+[ep][+]?[0-9]+`\n"
            "    * Base-16:  `[+-]?0x[0-9a-f]+[ep][+]?[0-9]+`\n"
            "    * Base-10:  `[+-]?[0-9]+[ep][+]?[0-9]+`\n"
            "    \n"
            "    If the string does not match any of the above, this function\n"
            "    fails. If the result is outside the range of representable\n"
            "    values of type `integer`, this function fails.\n"
            "  \n"
            "  * Returns the `integer` number converted from `text`. On failure,\n"
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
              Reference_Root::S_temporary xref = { std_numeric_parse_integer(text) };
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
            "    * Infinities:  `[+-]?infinity`\n"
            "    * NaNs:        `[+-]?nan`\n"
            "    * Base-2:      `[+-]?0b[01]+(\\.[01])?[ep][-+]?[0-9]+`\n"
            "    * Base-16:     `[+-]?0x[0-9a-f]+(\\.[0-9a-f])?[ep][-+]?[0-9]+`\n"
            "    * Base-10:     `[+-]?[0-9]+(\\.[0-9])?[ep][-+]?[0-9]+`\n"
            "    \n"
            "    If the string does not match any of the above, this function\n"
            "    fails. If the absolute value of the result is too small to fit\n"
            "    in a `real`, a signed zero is returned. When the absolute value\n"
            "    is too large, if `saturating` is set to `true`, a signed\n"
            "    infinity is returned; otherwise this function fails.\n"
            "  \n"
            "  * Returns the `real` number converted from `text`. On failure,\n"
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
              Reference_Root::S_temporary xref = { std_numeric_parse_real(text, saturating) };
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
