// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_numeric.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/random_number_generator.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

const Ival& do_verify_bounds(const Ival& lower, const Ival& upper)
  {
    if(!(lower <= upper)) {
      ASTERIA_THROW("bounds not valid (`$1` is not less than or equal to `$2`)", lower, upper);
    }
    return upper;
  }

const Rval& do_verify_bounds(const Rval& lower, const Rval& upper)
  {
    if(!::std::islessequal(lower, upper)) {
      ASTERIA_THROW("bounds not valid (`$1` is not less than or equal to `$2`)", lower, upper);
    }
    return upper;
  }

Ival do_cast_to_integer(const Rval& value)
  {
    if(!::std::islessequal(-0x1p63, value) || !::std::islessequal(value, 0x1p63 - 0x1p10)) {
      ASTERIA_THROW("`real` value not representable as an `integer` (value `$1`)", value);
    }
    return Ival(value);
  }

ROCKET_PURE_FUNCTION Ival do_saturating_add(const Ival& lhs, const Ival& rhs)
  {
    if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)) {
      return (rhs >> 63) ^ INT64_MAX;
    }
    return lhs + rhs;
  }

ROCKET_PURE_FUNCTION Ival do_saturating_sub(const Ival& lhs, const Ival& rhs)
  {
    if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)) {
      return (rhs >> 63) ^ INT64_MIN;
    }
    return lhs - rhs;
  }

ROCKET_PURE_FUNCTION Ival do_saturating_mul(const Ival& lhs, const Ival& rhs)
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
    // absolute lhs and signed rhs
    auto m = lhs >> 63;
    auto alhs = (lhs ^ m) - m;
    auto srhs = (rhs ^ m) - m;
    // `alhs` may only be positive here.
    if((srhs >= 0) ? (alhs > INT64_MAX / srhs) : (alhs > INT64_MIN / srhs)) {
      return (srhs >> 63) ^ INT64_MAX;
    }
    return alhs * srhs;
  }

ROCKET_PURE_FUNCTION Rval do_saturating_add(const Rval& lhs, const Rval& rhs)
  {
    return lhs + rhs;
  }

ROCKET_PURE_FUNCTION Rval do_saturating_sub(const Rval& lhs, const Rval& rhs)
  {
    return lhs - rhs;
  }

ROCKET_PURE_FUNCTION Rval do_saturating_mul(const Rval& lhs, const Rval& rhs)
  {
    return lhs * rhs;
  }

pair<Ival, int> do_decompose_integer(uint8_t ebase, const Ival& value)
  {
    int64_t ireg = value;
    int iexp = 0;
    for(;;) {
      if(ireg == 0)
        break;
      auto next = ireg / ebase;
      if(ireg % ebase != 0)
        break;
      ireg = next;
      iexp++;
    }
    return ::std::make_pair(ireg, iexp);
  }

Sval& do_append_exponent(Sval& text, ::rocket::ascii_numput& nump, char delim, int exp)
  {
    // Write the delimiter.
    text.push_back(delim);
    // If the exponent is non-negative, ensure there is a plus sign.
    if(exp >= 0)
      text.push_back('+');
    // Format the integer. If the exponent is negative, a minus sign will have been added.
    nump.put_DI(exp, 2);
    // Append significant figures.
    text.append(nump.begin(), nump.end());
    return text;
  }

constexpr char s_spaces[] = " \f\n\r\t\v";

}  // namespace

Ival std_numeric_abs(Ival value)
  {
    if(value == INT64_MIN) {
      ASTERIA_THROW("integer absolute value overflow (value `$1`)", value);
    }
    return ::std::abs(value);
  }

Rval std_numeric_abs(Rval value)
  {
    return ::std::fabs(value);
  }

Ival std_numeric_sign(Ival value)
  {
    return value >> 63;
  }

Ival std_numeric_sign(Rval value)
  {
    return ::std::signbit(value) ? -1 : 0;
  }

Bval std_numeric_is_finite(Ival /*value*/)
  {
    return true;
  }

Bval std_numeric_is_finite(Rval value)
  {
    return ::std::isfinite(value);
  }

Bval std_numeric_is_infinity(Ival /*value*/)
  {
    return false;
  }

Bval std_numeric_is_infinity(Rval value)
  {
    return ::std::isinf(value);
  }

Bval std_numeric_is_nan(Ival /*value*/)
  {
    return false;
  }

Bval std_numeric_is_nan(Rval value)
  {
    return ::std::isnan(value);
  }

Ival std_numeric_clamp(Ival value, Ival lower, Ival upper)
  {
    return ::rocket::clamp(value, lower, do_verify_bounds(lower, upper));
  }

Rval std_numeric_clamp(Rval value, Rval lower, Rval upper)
  {
    return ::rocket::clamp(value, lower, do_verify_bounds(lower, upper));
  }

Ival std_numeric_round(Ival value)
  {
    return value;
  }

Rval std_numeric_round(Rval value)
  {
    return ::std::round(value);
  }

Ival std_numeric_floor(Ival value)
  {
    return value;
  }

Rval std_numeric_floor(Rval value)
  {
    return ::std::floor(value);
  }

Ival std_numeric_ceil(Ival value)
  {
    return value;
  }

Rval std_numeric_ceil(Rval value)
  {
    return ::std::ceil(value);
  }

Ival std_numeric_trunc(Ival value)
  {
    return value;
  }

Rval std_numeric_trunc(Rval value)
  {
    return ::std::trunc(value);
  }

Ival std_numeric_iround(Ival value)
  {
    return value;
  }

Ival std_numeric_iround(Rval value)
  {
    return do_cast_to_integer(::std::round(value));
  }

Ival std_numeric_ifloor(Ival value)
  {
    return value;
  }

Ival std_numeric_ifloor(Rval value)
  {
    return do_cast_to_integer(::std::floor(value));
  }

Ival std_numeric_iceil(Ival value)
  {
    return value;
  }

Ival std_numeric_iceil(Rval value)
  {
    return do_cast_to_integer(::std::ceil(value));
  }

Ival std_numeric_itrunc(Ival value)
  {
    return value;
  }

Ival std_numeric_itrunc(Rval value)
  {
    return do_cast_to_integer(::std::trunc(value));
  }

Rval std_numeric_random(Global& global, Ropt limit)
  {
    auto prng = global.random_number_generator();

    // Generate a random `double` in the range [0.0,1.0).
    int64_t ireg = prng->bump();
    ireg <<= 21;
    ireg ^= prng->bump();
    double ratio = static_cast<double>(ireg) * 0x1p-53;

    // If a limit is specified, magnify the value.
    // The default magnitude is 1.0 so no action is taken.
    if(!limit) {
      return ratio;
    }
    switch(::std::fpclassify(*limit)) {
    case FP_ZERO:
      ASTERIA_THROW("random number limit shall not be zero");
    case FP_INFINITE:
    case FP_NAN:
      ASTERIA_THROW("random number limit shall be finite (limit `$1`)", *limit);
    }
    return *limit * ratio;
  }

Rval std_numeric_sqrt(Rval x)
  {
    return ::std::sqrt(x);
  }

Rval std_numeric_fma(Rval x, Rval y, Rval z)
  {
    return ::std::fma(x, y, z);
  }

Rval std_numeric_remainder(Rval x, Rval y)
  {
    return ::std::remainder(x, y);
  }

pair<Rval, Ival> std_numeric_frexp(Rval x)
  {
    int exp;
    auto frac = ::std::frexp(x, &exp);
    return ::std::make_pair(frac, exp);
  }

Rval std_numeric_ldexp(Rval frac, Ival exp)
  {
    int rexp = static_cast<int>(::rocket::clamp(exp, INT_MIN, INT_MAX));
    return ::std::ldexp(frac, rexp);
  }

Ival std_numeric_addm(Ival x, Ival y)
  {
    return Ival(static_cast<uint64_t>(x) + static_cast<uint64_t>(y));
  }

Ival std_numeric_subm(Ival x, Ival y)
  {
    return Ival(static_cast<uint64_t>(x) - static_cast<uint64_t>(y));
  }

Ival std_numeric_mulm(Ival x, Ival y)
  {
    return Ival(static_cast<uint64_t>(x) * static_cast<uint64_t>(y));
  }

Ival std_numeric_adds(Ival x, Ival y)
  {
    return do_saturating_add(x, y);
  }

Rval std_numeric_adds(Rval x, Rval y)
  {
    return do_saturating_add(x, y);
  }

Ival std_numeric_subs(Ival x, Ival y)
  {
    return do_saturating_sub(x, y);
  }

Rval std_numeric_subs(Rval x, Rval y)
  {
    return do_saturating_sub(x, y);
  }

Ival std_numeric_muls(Ival x, Ival y)
  {
    return do_saturating_mul(x, y);
  }

Rval std_numeric_muls(Rval x, Rval y)
  {
    return do_saturating_mul(x, y);
  }

Ival std_numeric_lzcnt(Ival x)
  {
    // TODO: Modern CPUs have intrinsics for this.
    uint64_t ireg = static_cast<uint64_t>(x);
    if(ireg == 0) {
      return 64;
    }
    uint32_t count = 0;
    // Scan bits from left to right.
    for(unsigned i = 32;  i != 0;  i /= 2) {
      if(ireg >> (64 - i))
        continue;
      ireg <<= i;
      count += i;
    }
    return count;
  }

Ival std_numeric_tzcnt(Ival x)
  {
    // TODO: Modern CPUs have intrinsics for this.
    uint64_t ireg = static_cast<uint64_t>(x);
    if(ireg == 0) {
      return 64;
    }
    uint32_t count = 0;
    // Scan bits from right to left.
    for(unsigned i = 32;  i != 0;  i /= 2) {
      if(ireg << (64 - i))
        continue;
      ireg >>= i;
      count += i;
    }
    return count;
  }

Ival std_numeric_popcnt(Ival x)
  {
    // TODO: Modern CPUs have intrinsics for this.
    uint64_t ireg = static_cast<uint64_t>(x);
    if(ireg == 0) {
      return 0;
    }
    uint32_t count = 0;
    // Scan bits from right to left.
    for(unsigned i = 0;  i < 64;  ++i) {
      uint32_t n = ireg & 1;
      ireg >>= 1;
      count += n;
    }
    return count;
  }

Ival std_numeric_rotl(Ival m, Ival x, Ival n)
  {
    if((m < 0) || (m > 64)) {
      ASTERIA_THROW("invalid modulo bit count (`$1` is not between 0 and 64)", m);
    }
    if(m == 0) {
      return 0;
    }
    uint64_t ireg = static_cast<uint64_t>(x);
    uint64_t mask = (UINT64_C(1) << (m - 1) << 1) - 1;
    // The shift count is modulo `m` so all values are defined.
    int64_t sh = n % m;
    if(sh != 0) {
      // Normalize the shift count.
      // Note that `sh + m` cannot be zero.
      if(sh < 0) {
        sh += m;
      }
      ireg = (ireg << sh) | ((ireg & mask) >> (m - sh));
    }
    // Clear the other bits.
    return static_cast<int64_t>(ireg & mask);
  }

Ival std_numeric_rotr(Ival m, Ival x, Ival n)
  {
    if((m < 0) || (m > 64)) {
      ASTERIA_THROW("invalid modulo bit count (`$1` is not between 0 and 64)", m);
    }
    if(m == 0) {
      return 0;
    }
    uint64_t ireg = static_cast<uint64_t>(x);
    uint64_t mask = (UINT64_C(1) << (m - 1) << 1) - 1;
    // The shift count is modulo `m` so all values are defined.
    int64_t sh = n % m;
    if(sh != 0) {
      // Normalize the shift count.
      // Note that `sh + m` cannot be zero.
      if(sh < 0) {
        sh += m;
      }
      ireg = ((ireg & mask) >> sh) | (ireg << (m - sh));
    }
    // Clear the other bits.
    return static_cast<int64_t>(ireg & mask);
  }

Sval std_numeric_format(Ival value, Iopt base, Iopt ebase)
  {
    Sval text;
    ::rocket::ascii_numput nump;

    switch(base.value_or(10)) {
    case 2: {
        if(!ebase) {
          nump.put_BI(value);  // binary, long
          text.append(nump.begin(), nump.end());
          break;
        }
        if(*ebase == 2) {
          auto p = do_decompose_integer(2, value);
          nump.put_BI(p.first);  // binary, long
          text.append(nump.begin(), nump.end());
          do_append_exponent(text, nump, 'p', p.second);
          break;
        }
        ASTERIA_THROW("invalid exponent base for binary notation (`$1` is not 2)", *ebase);
      }
    case 16: {
        if(!ebase) {
          nump.put_XI(value);  // hexadecimal, long
          text.append(nump.begin(), nump.end());
          break;
        }
        if(*ebase == 2) {
          auto p = do_decompose_integer(2, value);
          nump.put_XI(p.first);  // hexadecimal, long
          text.append(nump.begin(), nump.end());
          do_append_exponent(text, nump, 'p', p.second);
          break;
        }
        ASTERIA_THROW("invalid exponent base for hexadecimal notation (`$1` is not 2)", *ebase);
      }
    case 10: {
        if(!ebase) {
          nump.put_DI(value);  // decimal, long
          text.append(nump.begin(), nump.end());
          break;
        }
        if(*ebase == 10) {
          auto p = do_decompose_integer(10, value);
          nump.put_DI(p.first);  // decimal, long
          text.append(nump.begin(), nump.end());
          do_append_exponent(text, nump, 'e', p.second);
          break;
        }
        ASTERIA_THROW("invalid exponent base for decimal notation (`$1` is not 10)", *ebase);
      }
    default:
      ASTERIA_THROW("invalid number base (base `$1` is not one of { 2, 10, 16 })", *base);
    }
    return text;
  }

Sval std_numeric_format(Rval value, Iopt base, Iopt ebase)
  {
    Sval text;
    ::rocket::ascii_numput nump;

    switch(base.value_or(10)) {
    case 2: {
        if(!ebase) {
          nump.put_BF(value);  // binary, float
          text.append(nump.begin(), nump.end());
          break;
        }
        if(*ebase == 2) {
          nump.put_BE(value);  // binary, scientific
          text.append(nump.begin(), nump.end());
          break;
        }
        ASTERIA_THROW("invalid exponent base for binary notation (`$1` is not 2)", *ebase);
      }
    case 16: {
        if(!ebase) {
          nump.put_XF(value);  // hexadecimal, float
          text.append(nump.begin(), nump.end());
          break;
        }
        if(*ebase == 2) {
          nump.put_XE(value);  // hexadecimal, scientific
          text.append(nump.begin(), nump.end());
          break;
        }
        ASTERIA_THROW("invalid exponent base for hexadecimal notation (`$1` is not 2)", *ebase);
      }
    case 10: {
        if(!ebase) {
          nump.put_DF(value);  // decimal, float
          text.append(nump.begin(), nump.end());
          break;
        }
        if(*ebase == 10) {
          nump.put_DE(value);  // decimal, scientific
          text.append(nump.begin(), nump.end());
          break;
        }
        ASTERIA_THROW("invalid exponent base for decimal notation (`$1` is not 10)", *ebase);
      }
    default:
      ASTERIA_THROW("invalid number base (base `$1` is not one of { 2, 10, 16 })", *base);
    }
    return text;
  }

Ival std_numeric_parse_integer(Sval text)
  {
    auto tpos = text.find_first_not_of(s_spaces);
    if(tpos == Sval::npos) {
      ASTERIA_THROW("blank string");
    }
    auto bptr = text.data() + tpos;
    auto eptr = text.data() + text.find_last_not_of(s_spaces) + 1;

    Ival value;
    ::rocket::ascii_numget numg;
    if(!numg.parse_I(bptr, eptr)) {
      ASTERIA_THROW("string not convertible to integer (text `$1`)", text);
    }
    if(bptr != eptr) {
      ASTERIA_THROW("non-integer character in string (character `$1`)", *bptr);
    }
    if(!numg.cast_I(value, INT64_MIN, INT64_MAX)) {
      ASTERIA_THROW("integer overflow (text `$1`)", text);
    }
    return value;
  }

Rval std_numeric_parse_real(Sval text, Bopt saturating)
  {
    auto tpos = text.find_first_not_of(s_spaces);
    if(tpos == Sval::npos) {
      ASTERIA_THROW("blank string");
    }
    auto bptr = text.data() + tpos;
    auto eptr = text.data() + text.find_last_not_of(s_spaces) + 1;

    Rval value;
    ::rocket::ascii_numget numg;
    if(!numg.parse_F(bptr, eptr)) {
      ASTERIA_THROW("string not convertible to real number (text `$1`)", text);
    }
    if(bptr != eptr) {
      ASTERIA_THROW("non-real-number character in string (character `$1`)", *bptr);
    }
    if(!numg.cast_F(value, -HUGE_VAL, HUGE_VAL)) {
      // The value is out of range.
      // Unlike integers, underflows are accepted unconditionally.
      // Overflows are accepted unless `saturating` is `false` or absent.
      if(numg.overflowed() && (saturating != true))
        ASTERIA_THROW("real number overflow (text `$1`)", text);
    }
    return value;
  }

void create_bindings_numeric(Oval& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.numeric.integer_max`
    //===================================================================
    result.insert_or_assign(::rocket::sref("integer_max"),
      Ival(
        // The maximum value of an `integer`.
        ::std::numeric_limits<Ival>::max()
      ));
    //===================================================================
    // `std.numeric.integer_min`
    //===================================================================
    result.insert_or_assign(::rocket::sref("integer_min"),
      Ival(
        // The minimum value of an `integer`.
        ::std::numeric_limits<Ival>::lowest()
      ));
    //===================================================================
    // `std.numeric.real_max`
    //===================================================================
    result.insert_or_assign(::rocket::sref("real_max"),
      Rval(
        // The maximum finite value of a `real`.
        ::std::numeric_limits<Rval>::max()
      ));
    //===================================================================
    // `std.numeric.real_min`
    //===================================================================
    result.insert_or_assign(::rocket::sref("real_min"),
      Rval(
        // The minimum finite value of a `real`.
        ::std::numeric_limits<Rval>::lowest()
      ));
    //===================================================================
    // `std.numeric.real_epsilon`
    //===================================================================
    result.insert_or_assign(::rocket::sref("real_epsilon"),
      Rval(
        // The minimum finite value of a `real` such that `1 + real_epsilon > 1`.
        ::std::numeric_limits<Rval>::epsilon()
      ));
    //===================================================================
    // `std.numeric.size_max`
    //===================================================================
    result.insert_or_assign(::rocket::sref("size_max"),
      Ival(
        // The maximum length of a `string` or `array`.
        ::std::numeric_limits<ptrdiff_t>::max()
      ));
    //===================================================================
    // `std.numeric.abs()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("abs"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.abs(value)`\n"
          "\n"
          "  * Gets the absolute value of `value`, which may be an integer or\n"
          "    real. Negative integers are negated, which might cause an\n"
          "    exception to be thrown due to overflow. Sign bits of reals are\n"
          "    removed, which works on infinities and NaNs and does not\n"
          "    result in exceptions.\n"
          "\n"
          "  * Return the absolute value.\n"
          "\n"
          "  * Throws an exception if `value` is the integer `-0x1p63`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.abs"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_abs(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_abs(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.sign()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sign"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.sign(value)`\n"
          "\n"
          "  * Propagates the sign bit of the number `value`, which may be an\n"
          "    integer or real, to all bits of an integer. Be advised that\n"
          "    `-0.0` is distinct from `0.0` despite the equality.\n"
          "\n"
          "  * Returns `-1` if `value` is negative, or `0` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.sign"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_sign(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_sign(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.is_finite()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("is_finite"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.is_finite(value)`\n"
          "\n"
          "  * Checks whether `value` is a finite number. `value` may be an\n"
          "    integer or real. Be adviced that this functions returns `true`\n"
          "    for integers for consistency; integers do not support\n"
          "    infinities or NaNs.\n"
          "\n"
          "  * Returns `true` if `value` is an integer or is a real that\n"
          "    is neither an infinity or a NaN, or `false` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.is_finite"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_is_finite(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_is_finite(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.is_infinity()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("is_infinity"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.is_infinity(value)`\n"
          "\n"
          "  * Checks whether `value` is an infinity. `value` may be an\n"
          "    integer or real. Be adviced that this functions returns `false`\n"
          "    for integers for consistency; integers do not support\n"
          "    infinities.\n"
          "\n"
          "  * Returns `true` if `value` is a real that denotes an infinity;\n"
          "    or `false` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.is_infinity"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_is_infinity(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_is_infinity(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.is_nan()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("is_nan"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.is_nan(value)`\n"
          "\n"
          "  * Checks whether `value` is a NaN. `value` may be an integer or\n"
          "    real. Be adviced that this functions returns `false` for\n"
          "    integers for consistency; integers do not support NaNs.\n"
          "\n"
          "  * Returns `true` if `value` is a real denoting a NaN, or\n"
          "    `false` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.is_nan"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_is_nan(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_is_nan(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.clamp()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("clamp"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.clamp(value, lower, upper)`\n"
          "\n"
          "  * Limits `value` between `lower` and `upper`.\n"
          "\n"
          "  * Returns `lower` if `value < lower`, `upper` if `value > upper`,\n"
          "    or `value` otherwise, including when `value` is a NaN. The\n"
          "    returned value is an integer if all arguments are integers;\n"
          "    otherwise it is a real.\n"
          "\n"
          "  * Throws an exception if `lower` is not less than or equal to\n"
          "    `upper`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.clamp"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          Ival ilower;
          Ival iupper;
          if(reader.I().g(ivalue).g(ilower).g(iupper).F()) {
            // Call the binding function.
            return std_numeric_clamp(::rocket::move(ivalue), ::rocket::move(ilower), ::rocket::move(iupper));
          }
          Rval rvalue;
          Rval flower;
          Rval fupper;
          if(reader.I().g(rvalue).g(flower).g(fupper).F()) {
            // Call the binding function.
            return std_numeric_clamp(::rocket::move(rvalue), ::rocket::move(flower), ::rocket::move(fupper));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.round()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("round"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.round(value)`\n"
          "\n"
          "  * Rounds `value`, which may be an integer or real, to the nearest\n"
          "    integer; halfway values are rounded away from zero. If `value`\n"
          "    is an integer, it is returned intact.\n"
          "\n"
          "  * Returns the rounded value.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.round"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_round(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_round(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.floor()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("floor"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.floor(value)`\n"
          "\n"
          "  * Rounds `value`, which may be an integer or real, to the nearest\n"
          "    integer towards negative infinity. If `value` is an integer,\n"
          "    it is returned intact.\n"
          "\n"
          "  * Returns the rounded value.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.floor"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_floor(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_floor(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.ceil()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("ceil"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.ceil(value)`\n"
          "\n"
          "  * Rounds `value`, which may be an integer or real, to the nearest\n"
          "    integer towards positive infinity. If `value` is an integer, it\n"
          "    is returned intact.\n"
          "\n"
          "  * Returns the rounded value.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.ceil"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_ceil(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_ceil(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.trunc()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("trunc"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.trunc(value)`\n"
          "\n"
          "  * Rounds `value`, which may be an integer or real, to the nearest\n"
          "    integer towards zero. If `value` is an integer, it is returned\n"
          "    intact.\n"
          "\n"
          "  * Returns the rounded value.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.trunc"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_trunc(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_trunc(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.iround()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("iround"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.iround(value)`\n"
          "\n"
          "  * Rounds `value`, which may be an integer or real, to the nearest\n"
          "    integer; halfway values are rounded away from zero. If `value`\n"
          "    is an integer, it is returned intact. If `value` is a real, it\n"
          "    is converted to an integer.\n"
          "\n"
          "  * Returns the rounded value as an integer.\n"
          "\n"
          "  * Throws an exception if the result cannot be represented as an\n"
          "    integer.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.iround"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_iround(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_iround(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.ifloor()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("ifloor"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.ifloor(value)`\n"
          "\n"
          "  * Rounds `value`, which may be an integer or real, to the nearest\n"
          "    integer towards negative infinity. If `value` is an integer, it\n"
          "    is returned intact. If `value` is a real, it is converted to an\n"
          "    integer.\n"
          "\n"
          "  * Returns the rounded value as an integer.\n"
          "\n"
          "  * Throws an exception if the result cannot be represented as an\n"
          "    integer.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.ifloor"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_ifloor(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_ifloor(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.iceil()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("iceil"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.iceil(value)`\n"
          "\n"
          "  * Rounds `value`, which may be an integer or real, to the	\n"
          "    integer towards positive infinity. If `value` is an integer, it\n"
          "    is returned intact. If `value` is a real, it is converted to an\n"
          "    integer.\n"
          "\n"
          "  * Returns the rounded value as an integer.\n"
          "\n"
          "  * Throws an exception if the result cannot be represented as an\n"
          "    integer.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.iceil"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_iceil(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_iceil(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.itrunc()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("itrunc"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.itrunc(value)`\n"
          "\n"
          "  * Rounds `value`, which may be an integer or real, to the nearest\n"
          "    integer towards zero. If `value` is an integer, it is returned\n"
          "    intact. If `value` is a real, it is converted to an integer.\n"
          "\n"
          "  * Returns the rounded value as an integer.\n"
          "\n"
          "  * Throws an exception if the result cannot be represented as an\n"
          "    integer.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.itrunc"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          if(reader.I().g(ivalue).F()) {
            // Call the binding function.
            return std_numeric_itrunc(::rocket::move(ivalue));
          }
          Rval rvalue;
          if(reader.I().g(rvalue).F()) {
            // Call the binding function.
            return std_numeric_itrunc(::rocket::move(rvalue));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.random()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("random"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.random([limit])`\n"
          "\n"
          "  * Generates a random real value whose sign agrees with `limit`\n"
          "    and whose absolute value is less than `limit`. If `limit` is\n"
          "    absent, `1` is assumed.\n"
          "\n"
          "  * Returns a random real value.\n"
          "\n"
          "  * Throws an exception if `limit` is zero or non-finite.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.random"), ::rocket::ref(args));
          // Parse arguments.
          Ropt limit;
          if(reader.I().g(limit).F()) {
            // Call the binding function.
            return std_numeric_random(global, ::rocket::move(limit));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.sqrt()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sqrt"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.sqrt(x)`\n"
          "\n"
          "  * Calculates the square root of `x` which may be of either the\n"
          "    integer or the real type. The result is always a real.\n"
          "\n"
          "  * Returns the square root of `x` as a real.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.sqrt"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_numeric_sqrt(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.fma()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("fma"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.fma(x, y, z)`\n"
          "\n"
          "  * Performs fused multiply-add operation on `x`, `y` and `z`. This\n"
          "    functions calculates `x * y + z` without intermediate rounding\n"
          "    operations.\n"
          "\n"
          "  * Returns the value of `x * y + z` as a real.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.fma"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          Rval y;
          Rval z;
          if(reader.I().g(x).g(y).g(z).F()) {
            // Call the binding function.
            return std_numeric_fma(::rocket::move(x), ::rocket::move(y), ::rocket::move(z));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.remainder()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("remainder"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.remainder(x, y)`\n"
          "\n"
          "  * Calculates the IEEE floating-point remainder of division of `x`\n"
          "    by `y`. The remainder is defined to be `x - q * y` where `q` is\n"
          "    the quotient of division of `x` by `y` rounding to nearest.\n"
          "\n"
          "  * Returns the remainder as a real.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.remainder"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          Rval y;
          if(reader.I().g(x).g(y).F()) {
            // Call the binding function.
            return std_numeric_remainder(::rocket::move(x), ::rocket::move(y));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.frexp()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("frexp"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.frexp(x)`\n"
          "\n"
          "  * Decomposes `x` into normalized fractional and exponent parts\n"
          "    such that `x = frac * pow(2,exp)` where `frac` and `exp` denote\n"
          "    the fraction and the exponent respectively and `frac` is always\n"
          "    within the range `[0.5,1.0)`. If `x` is non-finite, `exp` is\n"
          "    unspecified.\n"
          "\n"
          "  * Returns an array having two elements, whose first element is\n"
          "    `frac` that is of type real and whose second element is `exp`\n"
          "    that is of type integer.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.frexp"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            auto pair = std_numeric_frexp(::rocket::move(x));
            // This function returns a `pair`, but we would like to return an array so convert it.
            Aval rval(2);
            rval.mut(0) = ::rocket::move(pair.first);
            rval.mut(1) = ::rocket::move(pair.second);
            return ::rocket::move(rval);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.ldexp()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("ldexp"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.ldexp(frac, exp)`\n"
          "\n"
          "  * Composes `frac` and `exp` to make a real number `x`, as if by\n"
          "    multiplying `frac` with `pow(2,exp)`. `exp` shall be of type\n"
          "    integer. This function is the inverse of `frexp()`.\n"
          "\n"
          "  * Returns the product as a real.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.ldexp"), ::rocket::ref(args));
          // Parse arguments.
          Rval frac;
          Ival exp;
          if(reader.I().g(frac).g(exp).F()) {
            // Call the binding function.
            return std_numeric_ldexp(::rocket::move(frac), ::rocket::move(exp));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.addm()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("addm"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.addm(x, y)`\n"
          "\n"
          "  * Adds `y` to `x` using modular arithmetic. `x` and `y` must be\n"
          "    of the integer type. The result is reduced to be congruent to\n"
          "    the sum of `x` and `y` modulo `0x1p64` in infinite precision.\n"
          "    This function will not cause overflow exceptions to be thrown.\n"
          "\n"
          "  * Returns the reduced sum of `x` and `y`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.addm"), ::rocket::ref(args));
          // Parse arguments.
          Ival x;
          Ival y;
          if(reader.I().g(x).g(y).F()) {
            // Call the binding function.
            return std_numeric_addm(::rocket::move(x), ::rocket::move(y));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.subm()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("subm"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.subm(x, y)`\n"
          "\n"
          "  * Subtracts `y` from `x` using modular arithmetic. `x` and `y`\n"
          "    must be of the integer type. The result is reduced to be\n"
          "    congruent to the difference of `x` and `y` modulo `0x1p64` in\n"
          "    infinite precision. This function will not cause overflow\n"
          "    exceptions to be thrown.\n"
          "\n"
          "  * Returns the reduced difference of `x` and `y`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.subm"), ::rocket::ref(args));
          // Parse arguments.
          Ival x;
          Ival y;
          if(reader.I().g(x).g(y).F()) {
            // Call the binding function.
            return std_numeric_subm(::rocket::move(x), ::rocket::move(y));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.mulm()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("mulm"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.mulm(x, y)`\n"
          "\n"
          "  * Multiplies `x` by `y` using modular arithmetic. `x` and `y`\n"
          "    must be of the integer type. The result is reduced to be\n"
          "    congruent to the product of `x` and `y` modulo `0x1p64` in\n"
          "    infinite precision. This function will not cause overflow\n"
          "    exceptions to be thrown.\n"
          "\n"
          "  * Returns the reduced product of `x` and `y`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.mulm"), ::rocket::ref(args));
          // Parse arguments.
          Ival x;
          Ival y;
          if(reader.I().g(x).g(y).F()) {
            // Call the binding function.
            return std_numeric_mulm(::rocket::move(x), ::rocket::move(y));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.adds()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("adds"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.adds(x, y)`\n"
          "\n"
          "  * Adds `y` to `x` using saturating arithmetic. `x` and `y` may be\n"
          "    integer or real values. The result is limited within the\n"
          "    range of representable values of its type, hence will not cause\n"
          "    overflow exceptions to be thrown. When either argument is of\n"
          "    type real which supports infinities, this function is\n"
          "    equivalent to the built-in addition operator.\n"
          "\n"
          "  * Returns the saturated sum of `x` and `y`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.adds"), ::rocket::ref(args));
          // Parse arguments.
          Ival ix;
          Ival iy;
          if(reader.I().g(ix).g(iy).F()) {
            // Call the binding function.
            return std_numeric_adds(::rocket::move(ix), ::rocket::move(iy));
          }
          Rval fx;
          Rval fy;
          if(reader.I().g(fx).g(fy).F()) {
            // Call the binding function.
            return std_numeric_adds(::rocket::move(fx), ::rocket::move(fy));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.subs()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("subs"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.subs(x, y)`\n"
          "\n"
          "  * Subtracts `y` from `x` using saturating arithmetic. `x` and `y`\n"
          "    may be integer or real values. The result is limited within the\n"
          "    range of representable values of its type, hence will not cause\n"
          "    overflow exceptions to be thrown. When either argument is of\n"
          "    type real which supports infinities, this function is\n"
          "    equivalent to the built-in subtraction operator.\n"
          "\n"
          "  * Returns the saturated difference of `x` and `y`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.subs"), ::rocket::ref(args));
          // Parse arguments.
          Ival ix;
          Ival iy;
          if(reader.I().g(ix).g(iy).F()) {
            // Call the binding function.
            return std_numeric_subs(::rocket::move(ix), ::rocket::move(iy));
          }
          Rval fx;
          Rval fy;
          if(reader.I().g(fx).g(fy).F()) {
            // Call the binding function.
            return std_numeric_subs(::rocket::move(fx), ::rocket::move(fy));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.muls()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("muls"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.muls(x, y)`\n"
          "\n"
          "  * Multiplies `x` by `y` using saturating arithmetic. `x` and `y`\n"
          "    may be integer or real values. The result is limited within the\n"
          "    range of representable values of its type, hence will not cause\n"
          "    overflow exceptions to be thrown. When either argument is of\n"
          "    type real which supports infinities, this function is\n"
          "    equivalent to the built-in multiplication operator.\n"
          "\n"
          "  * Returns the saturated product of `x` and `y`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.muls"), ::rocket::ref(args));
          // Parse arguments.
          Ival ix;
          Ival iy;
          if(reader.I().g(ix).g(iy).F()) {
            // Call the binding function.
            return std_numeric_muls(::rocket::move(ix), ::rocket::move(iy));
          }
          Rval fx;
          Rval fy;
          if(reader.I().g(fx).g(fy).F()) {
            // Call the binding function.
            return std_numeric_muls(::rocket::move(fx), ::rocket::move(fy));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.lzcnt()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("lzcnt"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.lzcnt(x)`\n"
          "\n"
          "  * Counts the number of leading zero bits in `x`, which shall be\n"
          "    of type integer.\n"
          "\n"
          "  * Returns the bit count as an integer. If `x` is zero, `64` is\n"
          "    returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.lzcnt"), ::rocket::ref(args));
          // Parse arguments.
          Ival x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_numeric_lzcnt(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.tzcnt()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("tzcnt"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.tzcnt(x)`\n"
          "\n"
          "  * Counts the number of trailing zero bits in `x`, which shall be\n"
          "    of type integer.\n"
          "\n"
          "  * Returns the bit count as an integer. If `x` is zero, `64` is\n"
          "    returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.tzcnt"), ::rocket::ref(args));
          // Parse arguments.
          Ival x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_numeric_tzcnt(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.popcnt()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("popcnt"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.popcnt(x)`\n"
          "\n"
          "  * Counts the number of one bits in `x`, which shall be of type\n"
          "    integer.\n"
          "\n"
          "  * Returns the bit count as an integer.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.popcnt"), ::rocket::ref(args));
          // Parse arguments.
          Ival x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_numeric_popcnt(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.rotl()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rotl"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.rotl(m, x, n)`\n"
          "\n"
          "  * Rotates the rightmost `m` bits of `x` to the left by `n`; all\n"
          "    arguments must be of type integer. This has the effect of\n"
          "    shifting `x` by `n` to the left then filling the vacuum in the\n"
          "    right with the last `n` bits that have just been shifted past\n"
          "    the left boundary. `n` is modulo `m` so rotating by a negative\n"
          "    count to the left has the same effect as rotating by its\n"
          "    absolute value to the right. All other bits are zeroed. If `m`\n"
          "    is zero, zero is returned.\n"
          "\n"
          "  * Returns the rotated value as an integer.\n"
          "\n"
          "  * Throws an exception if `m` is negative or greater than `64`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.rotl"), ::rocket::ref(args));
          // Parse arguments.
          Ival m;
          Ival x;
          Ival n;
          if(reader.I().g(m).g(x).g(n).F()) {
            // Call the binding function.
            return std_numeric_rotl(::rocket::move(m), ::rocket::move(x), ::rocket::move(n));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.rotr()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rotr"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.rotr(m, x, n)`\n"
          "\n"
          "  * Rotates the rightmost `m` bits of `x` to the right by `n`; all\n"
          "    arguments must be of type integer. This has the effect of\n"
          "    shifting `x` by `n` to the right then filling the vacuum in the\n"
          "    left with the last `n` bits that have just been shifted past\n"
          "    the right boundary. `n` is modulo `m` so rotating by a negative\n"
          "    count to the right has the same effect as rotating by its\n"
          "    absolute value to the left. All other bits are zeroed. If `m`\n"
          "    is zero, zero is returned.\n"
          "\n"
          "  * Returns the rotated value as an integer.\n"
          "\n"
          "  * Throws an exception if `m` is negative or greater than `64`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.rotr"), ::rocket::ref(args));
          // Parse arguments.
          Ival m;
          Ival x;
          Ival n;
          if(reader.I().g(m).g(x).g(n).F()) {
            // Call the binding function.
            return std_numeric_rotr(::rocket::move(m), ::rocket::move(x), ::rocket::move(n));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.format()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("format"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.format(value, [base], [exp_base])`\n"
          "\n"
          "  * Converts an integer or real number to a string in `base`. This\n"
          "    function writes as many digits as possible to ensure precision.\n"
          "    No plus sign precedes the significant figures. If `base` is\n"
          "    absent, `10` is assumed. If `ebase` is specified, an exponent\n"
          "    is appended to the significand as follows: If `value` is of\n"
          "    type integer, the significand is kept as short as possible;\n"
          "    otherwise (when `value` is of type real), it is written in\n"
          "    scientific notation. In both cases, the exponent comprises at\n"
          "    least two digits with an explicit sign. If `ebase` is absent,\n"
          "    no exponent appears. The result is exact as long as `base` is a\n"
          "    power of two.\n"
          "\n"
          "  * Returns a string converted from `value`.\n"
          "\n"
          "  * Throws an exception if `base` is neither `2` nor `10` nor `16`,\n"
          "    or if `ebase` is neither `2` nor `10`, or if `base` is not `10`\n"
          "    but `ebase` is `10`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.format"), ::rocket::ref(args));
          // Parse arguments.
          Ival ivalue;
          Iopt base;
          Iopt ebase;
          if(reader.I().g(ivalue).g(base).g(ebase).F()) {
            // Call the binding function.
            return std_numeric_format(::rocket::move(ivalue), ::rocket::move(base), ::rocket::move(ebase));
          }
          Rval fvalue;
          if(reader.I().g(fvalue).g(base).g(ebase).F()) {
            // Call the binding function.
            return std_numeric_format(::rocket::move(fvalue), ::rocket::move(base), ::rocket::move(ebase));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.parse_integer()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("parse_integer"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.parse_integer(text)`\n"
          "\n"
          "  * Parses `text` for an integer. `text` shall be a string. All\n"
          "    leading and trailing blank characters are stripped from `text`.\n"
          "    If it becomes empty, this function fails; otherwise, it shall\n"
          "    match one of the following extended regular expressions:\n"
          "\n"
          "    * Binary (base-2):\n"
          "      `[+-]?0[bB][01]+`\n"
          "    * Hexadecimal (base-16):\n"
          "      `[+-]?0[xX][0-9a-fA-F]+`\n"
          "    * Decimal (base-10):\n"
          "      `[+-]?[0-9]+`\n"
          "\n"
          "    If the string does not match any of the above, this function\n"
          "    fails. If the result is outside the range of representable\n"
          "    values of type integer, this function fails.\n"
          "\n"
          "  * Returns the integer value converted from `text`.\n"
          "\n"
          "  * Throws an exception on failure.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.parse_integer"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_numeric_parse_integer(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.numeric.parse_real()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("parse_real"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.numeric.parse_real(text, [saturating])`\n"
          "\n"
          "  * Parses `text` for a real number. `text` shall be a string. All\n"
          "    leading and trailing blank characters are stripped from `text`.\n"
          "    If it becomes empty, this function fails; otherwise, it shall\n"
          "    match any of the following extended regular expressions:\n"
          "\n"
          "    * Infinities:\n"
          "      `[+-]?infinity`\n"
          "    * NaNs:\n"
          "      `[+-]?nan`\n"
          "    * Binary (base-2):\n"
          "      `[+-]?0[bB][01]+(\\.[01]+)?([epEP][-+]?[0-9]+)?`\n"
          "    * Hexadecimal (base-16):\n"
          "      `[+-]?0x[0-9a-fA-F]+(\\.[0-9a-fA-F]+)?([epEP][-+]?[0-9]+)?`\n"
          "    * Decimal (base-10):\n"
          "      `[+-]?[0-9]+(\\.[0-9]+)?([eE][-+]?[0-9]+)?`\n"
          "\n"
          "    If the string does not match any of the above, this function\n"
          "    fails. If the absolute value of the result is too small to fit\n"
          "    in a real, a signed zero is returned. When the absolute value\n"
          "    is too large, if `saturating` is set to `true`, a signed\n"
          "    infinity is returned; otherwise this function fails.\n"
          "\n"
          "  * Returns the real value converted from `text`.\n"
          "\n"
          "  * Throws an exception on failure.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.numeric.parse_real"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Bopt saturating;
          if(reader.I().g(text).g(saturating).F()) {
            // Call the binding function.
            return std_numeric_parse_real(::rocket::move(text), ::rocket::move(saturating));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // End of `std.numeric`
    //===================================================================
  }

}  // namespace Asteria
