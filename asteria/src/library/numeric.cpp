// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "numeric.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/random_engine.hpp"
#include "../util.hpp"

namespace asteria {
namespace {

int64_t
do_verify_bounds(int64_t lower, int64_t upper)
  {
    if(!(lower <= upper))
      ASTERIA_THROW("Bounds not valid (`$1` is not less than or equal to `$2`)", lower, upper);
    return upper;
  }

double
do_verify_bounds(double lower, double upper)
  {
    if(!::std::islessequal(lower, upper))
      ASTERIA_THROW("Bounds not valid (`$1` is not less than or equal to `$2`)", lower, upper);
    return upper;
  }

V_integer
do_cast_to_integer(double value)
  {
    if(!::std::islessequal(-0x1p63, value) || !::std::islessequal(value, 0x1p63 - 0x1p10))
      ASTERIA_THROW("`real` value not representable as an `integer` (value `$1`)", value);
    return V_integer(value);
  }

ROCKET_CONST_FUNCTION
V_integer
do_saturating_add(int64_t lhs, int64_t rhs)
  {
    if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs))
      return (rhs >> 63) ^ INT64_MAX;
    return lhs + rhs;
  }

ROCKET_CONST_FUNCTION
V_integer
do_saturating_sub(int64_t lhs, int64_t rhs)
  {
    if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs))
      return (rhs >> 63) ^ INT64_MIN;
    return lhs - rhs;
  }

ROCKET_CONST_FUNCTION
V_integer
do_saturating_mul(int64_t lhs, int64_t rhs)
  {
    if((lhs == 0) || (rhs == 0))
      return 0;

    if((lhs == 1) || (rhs == 1))
      return (lhs ^ rhs) ^ 1;

    if((lhs == INT64_MIN) || (rhs == INT64_MIN))
      return (lhs >> 63) ^ (rhs >> 63) ^ INT64_MAX;

    if((lhs == -1) || (rhs == -1))
      return (lhs ^ rhs) + 1;

    // absolute lhs and signed rhs
    auto m = lhs >> 63;
    auto alhs = (lhs ^ m) - m;
    auto srhs = (rhs ^ m) - m;
    // `alhs` may only be positive here.
    if((srhs >= 0) ? (alhs > INT64_MAX / srhs) : (alhs > INT64_MIN / srhs))
      return (srhs >> 63) ^ INT64_MAX;
    return alhs * srhs;
  }

ROCKET_CONST_FUNCTION
V_real
do_saturating_add(double lhs, double rhs)
  {
    return lhs + rhs;
  }

ROCKET_CONST_FUNCTION
V_real
do_saturating_sub(double lhs, double rhs)
  {
    return lhs - rhs;
  }

ROCKET_CONST_FUNCTION
V_real
do_saturating_mul(double lhs, double rhs)
  {
    return lhs * rhs;
  }

pair<V_integer, int>
do_decompose_integer(uint8_t ebase, int64_t value)
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

V_string&
do_append_exponent(V_string& text, ::rocket::ascii_numput& nump, char delim, int exp)
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

V_integer
std_numeric_abs(V_integer value)
  {
    if(value == INT64_MIN)
      ASTERIA_THROW("Integer absolute value overflow (value `$1`)", value);
    return ::std::abs(value);
  }

V_real
std_numeric_abs(V_real value)
  {
    return ::std::fabs(value);
  }

V_integer
std_numeric_sign(V_integer value)
  {
    return value >> 63;
  }

V_integer
std_numeric_sign(V_real value)
  {
    return ::std::signbit(value) ? -1 : 0;
  }

V_boolean
std_numeric_is_finite(V_integer /*value*/)
  {
    return true;
  }

V_boolean
std_numeric_is_finite(V_real value)
  {
    return ::std::isfinite(value);
  }

V_boolean
std_numeric_is_infinity(V_integer /*value*/)
  {
    return false;
  }

V_boolean
std_numeric_is_infinity(V_real value)
  {
    return ::std::isinf(value);
  }

V_boolean
std_numeric_is_nan(V_integer /*value*/)
  {
    return false;
  }

V_boolean
std_numeric_is_nan(V_real value)
  {
    return ::std::isnan(value);
  }

V_integer
std_numeric_clamp(V_integer value, V_integer lower, V_integer upper)
  {
    return ::rocket::clamp(value, lower, do_verify_bounds(lower, upper));
  }

V_real
std_numeric_clamp(V_real value, V_real lower, V_real upper)
  {
    return ::rocket::clamp(value, lower, do_verify_bounds(lower, upper));
  }

V_integer
std_numeric_round(V_integer value)
  {
    return value;
  }

V_real
std_numeric_round(V_real value)
  {
    return ::std::round(value);
  }

V_integer
std_numeric_iround(V_integer value)
  {
    return value;
  }

V_integer
std_numeric_iround(V_real value)
  {
    return do_cast_to_integer(::std::round(value));
  }

V_integer
std_numeric_floor(V_integer value)
  {
    return value;
  }

V_real
std_numeric_floor(V_real value)
  {
    return ::std::floor(value);
  }

V_integer
std_numeric_ifloor(V_integer value)
  {
    return value;
  }

V_integer
std_numeric_ifloor(V_real value)
  {
    return do_cast_to_integer(::std::floor(value));
  }

V_integer
std_numeric_ceil(V_integer value)
  {
    return value;
  }

V_real
std_numeric_ceil(V_real value)
  {
    return ::std::ceil(value);
  }

V_integer
std_numeric_iceil(V_integer value)
  {
    return value;
  }

V_integer
std_numeric_iceil(V_real value)
  {
    return do_cast_to_integer(::std::ceil(value));
  }

V_integer
std_numeric_trunc(V_integer value)
  {
    return value;
  }

V_real
std_numeric_trunc(V_real value)
  {
    return ::std::trunc(value);
  }

V_integer
std_numeric_itrunc(V_integer value)
  {
    return value;
  }

V_integer
std_numeric_itrunc(V_real value)
  {
    return do_cast_to_integer(::std::trunc(value));
  }

V_real
std_numeric_random(Global_Context& global, Opt_real limit)
  {
    auto prng = global.random_engine();

    // Generate a random `double` in the range [0.0,1.0).
    int64_t ireg = prng->bump();
    ireg <<= 21;
    ireg ^= prng->bump();
    double ratio = static_cast<double>(ireg) * 0x1p-53;

    // If a limit is specified, magnify the value.
    // The default magnitude is 1.0 so no action is taken.
    if(limit) {
      switch(::std::fpclassify(*limit)) {
        case FP_ZERO:
          ASTERIA_THROW("Random number limit shall not be zero");

        case FP_INFINITE:
        case FP_NAN:
          ASTERIA_THROW("Random number limit shall be finite (limit `$1`)", *limit);

        default:
          ratio *= *limit;
      }
    }
    return ratio;
  }

V_real
std_numeric_sqrt(V_real x)
  {
    return ::std::sqrt(x);
  }

V_real
std_numeric_fma(V_real x, V_real y, V_real z)
  {
    return ::std::fma(x, y, z);
  }

V_real
std_numeric_remainder(V_real x, V_real y)
  {
    return ::std::remainder(x, y);
  }

pair<V_real, V_integer>
std_numeric_frexp(V_real x)
  {
    int exp;
    auto frac = ::std::frexp(x, &exp);
    return ::std::make_pair(frac, exp);
  }

V_real
std_numeric_ldexp(V_real frac, V_integer exp)
  {
    int rexp = static_cast<int>(::rocket::clamp(exp, INT_MIN, INT_MAX));
    return ::std::ldexp(frac, rexp);
  }

V_integer
std_numeric_addm(V_integer x, V_integer y)
  {
    return V_integer(static_cast<uint64_t>(x) + static_cast<uint64_t>(y));
  }

V_integer
std_numeric_subm(V_integer x, V_integer y)
  {
    return V_integer(static_cast<uint64_t>(x) - static_cast<uint64_t>(y));
  }

V_integer
std_numeric_mulm(V_integer x, V_integer y)
  {
    return V_integer(static_cast<uint64_t>(x) * static_cast<uint64_t>(y));
  }

V_integer
std_numeric_adds(V_integer x, V_integer y)
  {
    return do_saturating_add(x, y);
  }

V_real
std_numeric_adds(V_real x, V_real y)
  {
    return do_saturating_add(x, y);
  }

V_integer
std_numeric_subs(V_integer x, V_integer y)
  {
    return do_saturating_sub(x, y);
  }

V_real
std_numeric_subs(V_real x, V_real y)
  {
    return do_saturating_sub(x, y);
  }

V_integer
std_numeric_muls(V_integer x, V_integer y)
  {
    return do_saturating_mul(x, y);
  }

V_real
std_numeric_muls(V_real x, V_real y)
  {
    return do_saturating_mul(x, y);
  }

V_integer
std_numeric_lzcnt(V_integer x)
  {
    if(ROCKET_UNEXPECT(x == 0))
      return 64;
    else
      return ROCKET_LZCNT64_NZ(static_cast<uint64_t>(x));
  }

V_integer
std_numeric_tzcnt(V_integer x)
  {
    if(ROCKET_UNEXPECT(x == 0))
      return 64;
    else
      return ROCKET_TZCNT64_NZ(static_cast<uint64_t>(x));
  }

V_integer
std_numeric_popcnt(V_integer x)
  {
    return ROCKET_POPCNT64(static_cast<uint64_t>(x));
  }

V_integer
std_numeric_rotl(V_integer m, V_integer x, V_integer n)
  {
    if((m < 0) || (m > 64))
      ASTERIA_THROW("Invalid modulo bit count (`$1` is not between 0 and 64)", m);

    if(m == 0)
      return 0;

    // The shift count is modulo `m` so all values are defined.
    uint64_t ireg = static_cast<uint64_t>(x);
    uint64_t mask = (UINT64_C(1) << (m - 1) << 1) - 1;
    int64_t sh = n % m;
    if(sh != 0) {
      // Normalize the shift count.
      // Note that `sh + m` cannot be zero.
      sh += (sh >> 63) & m;
      ireg = (ireg << sh) | ((ireg & mask) >> (m - sh));
    }
    // Clear the other bits.
    return static_cast<int64_t>(ireg & mask);
  }

V_integer
std_numeric_rotr(V_integer m, V_integer x, V_integer n)
  {
    if((m < 0) || (m > 64))
      ASTERIA_THROW("Invalid modulo bit count (`$1` is not between 0 and 64)", m);

    if(m == 0)
      return 0;

    // The shift count is modulo `m` so all values are defined.
    uint64_t ireg = static_cast<uint64_t>(x);
    uint64_t mask = (UINT64_C(1) << (m - 1) << 1) - 1;
    int64_t sh = n % m;
    if(sh != 0) {
      // Normalize the shift count.
      // Note that `sh + m` cannot be zero.
      sh += (sh >> 63) & m;
      ireg = ((ireg & mask) >> sh) | (ireg << (m - sh));
    }
    // Clear the other bits.
    return static_cast<int64_t>(ireg & mask);
  }

V_string
std_numeric_format(V_integer value, Opt_integer base, Opt_integer ebase)
  {
    V_string text;
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

        ASTERIA_THROW("Invalid exponent base for binary notation (`$1` is not 2)", *ebase);
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

        ASTERIA_THROW("Invalid exponent base for hexadecimal notation (`$1` is not 2)", *ebase);
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

        ASTERIA_THROW("Invalid exponent base for decimal notation (`$1` is not 10)", *ebase);
      }

      default:
        ASTERIA_THROW("Invalid number base (base `$1` is not one of { 2, 10, 16 })", *base);
    }
    return text;
  }

V_string
std_numeric_format(V_real value, Opt_integer base, Opt_integer ebase)
  {
    V_string text;
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
        ASTERIA_THROW("Invalid exponent base for binary notation (`$1` is not 2)", *ebase);
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

        ASTERIA_THROW("Invalid exponent base for hexadecimal notation (`$1` is not 2)", *ebase);
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

        ASTERIA_THROW("Invalid exponent base for decimal notation (`$1` is not 10)", *ebase);
      }

      default:
        ASTERIA_THROW("Invalid number base (base `$1` is not one of { 2, 10, 16 })", *base);
    }
    return text;
  }

V_integer
std_numeric_parse_integer(V_string text)
  {
    auto tpos = text.find_first_not_of(s_spaces);
    if(tpos == V_string::npos)
      ASTERIA_THROW("Blank string");
    auto bptr = text.data() + tpos;
    auto eptr = text.data() + text.find_last_not_of(s_spaces) + 1;

    V_integer value;
    ::rocket::ascii_numget numg;
    if(!numg.parse_I(bptr, eptr))
      ASTERIA_THROW("String not convertible to integer (text `$1`)", text);

    if(bptr != eptr)
      ASTERIA_THROW("Non-integer character in string (character `$1`)", *bptr);

    if(!numg.cast_I(value, INT64_MIN, INT64_MAX))
      ASTERIA_THROW("Integer overflow (text `$1`)", text);
    return value;
  }

V_real
std_numeric_parse_real(V_string text, Opt_boolean saturating)
  {
    auto tpos = text.find_first_not_of(s_spaces);
    if(tpos == V_string::npos)
      ASTERIA_THROW("Blank string");
    auto bptr = text.data() + tpos;
    auto eptr = text.data() + text.find_last_not_of(s_spaces) + 1;

    V_real value;
    ::rocket::ascii_numget numg;
    if(!numg.parse_F(bptr, eptr))
      ASTERIA_THROW("String not convertible to real number (text `$1`)", text);

    if(bptr != eptr)
      ASTERIA_THROW("Non-real-number character in string (character `$1`)", *bptr);

    if(!numg.cast_F(value, -HUGE_VAL, HUGE_VAL)) {
      // The value is out of range.
      // Unlike integers, underflows are accepted unconditionally.
      // Overflows are accepted unless `saturating` is `false` or absent.
      if(numg.overflowed() && (saturating != true))
        ASTERIA_THROW("Real number overflow (text `$1`)", text);
    }
    return value;
  }

void
create_bindings_numeric(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.numeric.integer_max`
    //===================================================================
    result.insert_or_assign(::rocket::sref("integer_max"),
      V_integer(
        // The maximum value of an `integer`.
        ::std::numeric_limits<V_integer>::max()
      ));

    //===================================================================
    // `std.numeric.integer_min`
    //===================================================================
    result.insert_or_assign(::rocket::sref("integer_min"),
      V_integer(
        // The minimum value of an `integer`.
        ::std::numeric_limits<V_integer>::lowest()
      ));

    //===================================================================
    // `std.numeric.real_max`
    //===================================================================
    result.insert_or_assign(::rocket::sref("real_max"),
      V_real(
        // The maximum finite value of a `real`.
        ::std::numeric_limits<V_real>::max()
      ));

    //===================================================================
    // `std.numeric.real_min`
    //===================================================================
    result.insert_or_assign(::rocket::sref("real_min"),
      V_real(
        // The minimum finite value of a `real`.
        ::std::numeric_limits<V_real>::lowest()
      ));

    //===================================================================
    // `std.numeric.real_epsilon`
    //===================================================================
    result.insert_or_assign(::rocket::sref("real_epsilon"),
      V_real(
        // The minimum finite value of a `real` such that `1 + real_epsilon > 1`.
        ::std::numeric_limits<V_real>::epsilon()
      ));

    //===================================================================
    // `std.numeric.size_max`
    //===================================================================
    result.insert_or_assign(::rocket::sref("size_max"),
      V_integer(
        // The maximum length of a `string` or `array`.
        ::std::numeric_limits<ptrdiff_t>::max()
      ));

    //===================================================================
    // `std.numeric.abs()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("abs"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.abs(value)`

  * Gets the absolute value of `value`, which may be an integer or
    real. Negative integers are negated, which might cause an
    exception to be thrown due to overflow. Sign bits of reals are
    removed, which works on infinities and NaNs and does not
    result in exceptions.

  * Return the absolute value.

  * Throws an exception if `value` is the integer `-0x1p63`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.abs"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_abs(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_abs(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.sign()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sign"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.sign(value)`

  * Propagates the sign bit of the number `value`, which may be an
    integer or real, to all bits of an integer. Be advised that
    `-0.0` is distinct from `0.0` despite the equality.

  * Returns `-1` if `value` is negative, or `0` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.sign"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_sign(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_sign(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.is_finite()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("is_finite"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.is_finite(value)`

  * Checks whether `value` is a finite number. `value` may be an
    integer or real. Be adviced that this functions returns `true`
    for integers for consistency; integers do not support
    infinities or NaNs.

  * Returns `true` if `value` is an integer or is a real that
    is neither an infinity or a NaN, or `false` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.is_finite"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_is_finite(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_is_finite(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.is_infinity()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("is_infinity"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.is_infinity(value)`

  * Checks whether `value` is an infinity. `value` may be an
    integer or real. Be adviced that this functions returns `false`
    for integers for consistency; integers do not support
    infinities.

  * Returns `true` if `value` is a real that denotes an infinity;
    or `false` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.is_infinity"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_is_infinity(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_is_infinity(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.is_nan()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("is_nan"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.is_nan(value)`

  * Checks whether `value` is a NaN. `value` may be an integer or
    real. Be adviced that this functions returns `false` for
    integers for consistency; integers do not support NaNs.

  * Returns `true` if `value` is a real denoting a NaN, or
    `false` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.is_nan"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_is_nan(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_is_nan(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.clamp()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("clamp"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.clamp(value, lower, upper)`

  * Limits `value` between `lower` and `upper`.

  * Returns `lower` if `value < lower`, `upper` if `value > upper`,
    or `value` otherwise, including when `value` is a NaN. The
    returned value is an integer if all arguments are integers;
    otherwise it is a real.

  * Throws an exception if `lower` is not less than or equal to
    `upper`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.clamp"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    V_integer ilower;
    V_integer iupper;
    if(reader.I().v(ivalue).v(ilower).v(iupper).F()) {
      Reference::S_temporary xref = { std_numeric_clamp(::std::move(ivalue), ::std::move(ilower),
                                                             ::std::move(iupper)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    V_real flower;
    V_real fupper;
    if(reader.I().v(rvalue).v(flower).v(fupper).F()) {
      Reference::S_temporary xref = { std_numeric_clamp(::std::move(rvalue), ::std::move(flower),
                                                             ::std::move(fupper)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.round()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("round"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.round(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer; halfway values are rounded away from zero. If `value`
    is an integer, it is returned intact.

  * Returns the rounded value.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.round"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_round(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_round(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.iround()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("iround"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.iround(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer; halfway values are rounded away from zero. If `value`
    is an integer, it is returned intact. If `value` is a real, it
    is converted to an integer.

  * Returns the rounded value as an integer.

  * Throws an exception if the result cannot be represented as an
    integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.iround"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_iround(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_iround(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.floor()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("floor"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.floor(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer towards negative infinity. If `value` is an integer, it
    is returned intact.

  * Returns the rounded value.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.floor"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_floor(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_floor(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.ifloor()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("ifloor"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.ifloor(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer towards negative infinity. If `value` is an integer, it
    is returned intact. If `value` is a real, it is converted to an
    integer.

  * Returns the rounded value as an integer.

  * Throws an exception if the result cannot be represented as an
    integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.ifloor"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_ifloor(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_ifloor(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.ceil()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("ceil"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.ceil(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer towards positive infinity. If `value` is an integer,
    it is returned intact.

  * Returns the rounded value.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.ceil"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_ceil(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_ceil(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.iceil()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("iceil"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.iceil(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer towards positive infinity. If `value` is an integer, it
    is returned intact. If `value` is a real, it is converted to an
    integer.

  * Returns the rounded value as an integer.

  * Throws an exception if the result cannot be represented as an
    integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.iceil"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_iceil(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_iceil(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.trunc()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("trunc"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.trunc(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer towards zero. If `value` is an integer, it is returned
    intact.

  * Returns the rounded value.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.trunc"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_trunc(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_trunc(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.itrunc()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("itrunc"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.itrunc(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer towards zero. If `value` is an integer, it is returned
    intact. If `value` is a real, it is converted to an integer.

  * Returns the rounded value as an integer.

  * Throws an exception if the result cannot be represented as an
    integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.itrunc"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference::S_temporary xref = { std_numeric_itrunc(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    V_real rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference::S_temporary xref = { std_numeric_itrunc(::std::move(rvalue)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.random()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("random"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.random([limit])`

  * Generates a random real value whose sign agrees with `limit`
    and whose absolute value is less than `limit`. If `limit` is
    absent, `1` is assumed.

  * Returns a random real value.

  * Throws an exception if `limit` is zero or non-finite.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& global, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.random"), ::rocket::cref(args));
    // Parse arguments.
    Opt_real limit;
    if(reader.I().o(limit).F()) {
      Reference::S_temporary xref = { std_numeric_random(global, ::std::move(limit)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.sqrt()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sqrt"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.sqrt(x)`

  * Calculates the square root of `x` which may be of either the
    integer or the real type. The result is always a real.

  * Returns the square root of `x` as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.sqrt"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference::S_temporary xref = { std_numeric_sqrt(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.fma()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("fma"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.fma(x, y, z)`

  * Performs fused multiply-add operation on `x`, `y` and `z`. This
    functions calculates `x * y + z` without intermediate rounding
    operations.

  * Returns the value of `x * y + z` as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.fma"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    V_real y;
    V_real z;
    if(reader.I().v(x).v(y).v(z).F()) {
      Reference::S_temporary xref = { std_numeric_fma(::std::move(x), ::std::move(y), ::std::move(z)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.remainder()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("remainder"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.remainder(x, y)`

  * Calculates the IEEE floating-point remainder of division of `x`
    by `y`. The remainder is defined to be `x - q * y` where `q` is
    the quotient of division of `x` by `y` rounding to nearest.

  * Returns the remainder as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.remainder"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    V_real y;
    if(reader.I().v(x).v(y).F()) {
      Reference::S_temporary xref = { std_numeric_remainder(::std::move(x), ::std::move(y)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.frexp()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("frexp"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.frexp(x)`

  * Decomposes `x` into normalized fractional and exponent parts
    such that `x = frac * pow(2,exp)` where `frac` and `exp` denote
    the fraction and the exponent respectively and `frac` is always
    within the range `[0.5,1.0)`. If `x` is non-finite, `exp` is
    unspecified.

  * Returns an array having two elements, whose first element is
    `frac` that is of type real and whose second element is `exp`
    that is of type integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.frexp"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference::S_temporary xref = { std_numeric_frexp(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.ldexp()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("ldexp"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.ldexp(frac, exp)`

  * Composes `frac` and `exp` to make a real number `x`, as if by
    multiplying `frac` with `pow(2,exp)`. `exp` shall be of type
    integer. This function is the inverse of `frexp()`.

  * Returns the product as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.ldexp"), ::rocket::cref(args));
    // Parse arguments.
    V_real frac;
    V_integer exp;
    if(reader.I().v(frac).v(exp).F()) {
      Reference::S_temporary xref = { std_numeric_ldexp(::std::move(frac), ::std::move(exp)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.addm()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("addm"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.addm(x, y)`

  * Adds `y` to `x` using modular arithmetic. `x` and `y` must be
    of the integer type. The result is reduced to be congruent to
    the sum of `x` and `y` modulo `0x1p64` in infinite precision.
    This function will not cause overflow exceptions to be thrown.

  * Returns the reduced sum of `x` and `y`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.addm"), ::rocket::cref(args));
    // Parse arguments.
    V_integer x;
    V_integer y;
    if(reader.I().v(x).v(y).F()) {
      Reference::S_temporary xref = { std_numeric_addm(::std::move(x), ::std::move(y)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.subm()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("subm"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.subm(x, y)`

  * Subtracts `y` from `x` using modular arithmetic. `x` and `y`
    must be of the integer type. The result is reduced to be
    congruent to the difference of `x` and `y` modulo `0x1p64` in
    infinite precision. This function will not cause overflow
    exceptions to be thrown.

  * Returns the reduced difference of `x` and `y`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.subm"), ::rocket::cref(args));
    // Parse arguments.
    V_integer x;
    V_integer y;
    if(reader.I().v(x).v(y).F()) {
      Reference::S_temporary xref = { std_numeric_subm(::std::move(x), ::std::move(y)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.mulm()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("mulm"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.mulm(x, y)`

  * Multiplies `x` by `y` using modular arithmetic. `x` and `y`
    must be of the integer type. The result is reduced to be
    congruent to the product of `x` and `y` modulo `0x1p64` in
    infinite precision. This function will not cause overflow
    exceptions to be thrown.

  * Returns the reduced product of `x` and `y`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.mulm"), ::rocket::cref(args));
    // Parse arguments.
    V_integer x;
    V_integer y;
    if(reader.I().v(x).v(y).F()) {
      Reference::S_temporary xref = { std_numeric_mulm(::std::move(x), ::std::move(y)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.adds()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("adds"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.adds(x, y)`

  * Adds `y` to `x` using saturating arithmetic. `x` and `y` may be
    integer or real values. The result is limited within the
    range of representable values of its type, hence will not cause
    overflow exceptions to be thrown. When either argument is of
    type real which supports infinities, this function is
    equivalent to the built-in addition operator.

  * Returns the saturated sum of `x` and `y`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.adds"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ix;
    V_integer iy;
    if(reader.I().v(ix).v(iy).F()) {
      Reference::S_temporary xref = { std_numeric_adds(::std::move(ix), ::std::move(iy)) };
      return self = ::std::move(xref);
    }
    V_real fx;
    V_real fy;
    if(reader.I().v(fx).v(fy).F()) {
      Reference::S_temporary xref = { std_numeric_adds(::std::move(fx), ::std::move(fy)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.subs()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("subs"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.subs(x, y)`

  * Subtracts `y` from `x` using saturating arithmetic. `x` and `y`
    may be integer or real values. The result is limited within the
    range of representable values of its type, hence will not cause
    overflow exceptions to be thrown. When either argument is of
    type real which supports infinities, this function is
    equivalent to the built-in subtraction operator.

  * Returns the saturated difference of `x` and `y`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.subs"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ix;
    V_integer iy;
    if(reader.I().v(ix).v(iy).F()) {
      Reference::S_temporary xref = { std_numeric_subs(::std::move(ix), ::std::move(iy)) };
      return self = ::std::move(xref);
    }
    V_real fx;
    V_real fy;
    if(reader.I().v(fx).v(fy).F()) {
      Reference::S_temporary xref = { std_numeric_subs(::std::move(fx), ::std::move(fy)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.muls()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("muls"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.muls(x, y)`

  * Multiplies `x` by `y` using saturating arithmetic. `x` and `y`
    may be integer or real values. The result is limited within the
    range of representable values of its type, hence will not cause
    overflow exceptions to be thrown. When either argument is of
    type real which supports infinities, this function is
    equivalent to the built-in multiplication operator.

  * Returns the saturated product of `x` and `y`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.muls"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ix;
    V_integer iy;
    if(reader.I().v(ix).v(iy).F()) {
      Reference::S_temporary xref = { std_numeric_muls(::std::move(ix), ::std::move(iy)) };
      return self = ::std::move(xref);
    }
    V_real fx;
    V_real fy;
    if(reader.I().v(fx).v(fy).F()) {
      Reference::S_temporary xref = { std_numeric_muls(::std::move(fx), ::std::move(fy)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.lzcnt()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("lzcnt"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.lzcnt(x)`

  * Counts the number of leading zero bits in `x`, which shall be
    of type integer.

  * Returns the bit count as an integer. If `x` is zero, `64` is
    returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.lzcnt"), ::rocket::cref(args));
    // Parse arguments.
    V_integer x;
    if(reader.I().v(x).F()) {
      Reference::S_temporary xref = { std_numeric_lzcnt(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.tzcnt()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("tzcnt"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.tzcnt(x)`

  * Counts the number of trailing zero bits in `x`, which shall be
    of type integer.

  * Returns the bit count as an integer. If `x` is zero, `64` is
    returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.tzcnt"), ::rocket::cref(args));
    // Parse arguments.
    V_integer x;
    if(reader.I().v(x).F()) {
      Reference::S_temporary xref = { std_numeric_tzcnt(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.popcnt()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("popcnt"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.popcnt(x)`

  * Counts the number of one bits in `x`, which shall be of type
    integer.

  * Returns the bit count as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.popcnt"), ::rocket::cref(args));
    // Parse arguments.
    V_integer x;
    if(reader.I().v(x).F()) {
      Reference::S_temporary xref = { std_numeric_popcnt(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.rotl()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rotl"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.rotl(m, x, n)`

  * Rotates the rightmost `m` bits of `x` to the left by `n`; all
    arguments must be of type integer. This has the effect of
    shifting `x` by `n` to the left then filling the vacuum in the
    right with the last `n` bits that have just been shifted past
    the left boundary. `n` is modulo `m` so rotating by a negative
    count to the left has the same effect as rotating by its
    absolute value to the right. All other bits are zeroed. If `m`
    is zero, zero is returned.

  * Returns the rotated value as an integer.

  * Throws an exception if `m` is negative or greater than `64`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.rotl"), ::rocket::cref(args));
    // Parse arguments.
    V_integer m;
    V_integer x;
    V_integer n;
    if(reader.I().v(m).v(x).v(n).F()) {
      Reference::S_temporary xref = { std_numeric_rotl(::std::move(m), ::std::move(x), ::std::move(n)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.rotr()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rotr"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.rotr(m, x, n)`

  * Rotates the rightmost `m` bits of `x` to the right by `n`; all
    arguments must be of type integer. This has the effect of
    shifting `x` by `n` to the right then filling the vacuum in the
    left with the last `n` bits that have just been shifted past
    the right boundary. `n` is modulo `m` so rotating by a negative
    count to the right has the same effect as rotating by its
    absolute value to the left. All other bits are zeroed. If `m`
    is zero, zero is returned.

  * Returns the rotated value as an integer.

  * Throws an exception if `m` is negative or greater than `64`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.rotr"), ::rocket::cref(args));
    // Parse arguments.
    V_integer m;
    V_integer x;
    V_integer n;
    if(reader.I().v(m).v(x).v(n).F()) {
      Reference::S_temporary xref = { std_numeric_rotr(::std::move(m), ::std::move(x), ::std::move(n)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.format()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("format"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.format(value, [base], [ebase])`

  * Converts an integer or real number to a string in `base`. This
    function writes as many digits as possible to ensure precision.
    No plus sign precedes the significant figures. If `base` is
    absent, `10` is assumed. If `ebase` is specified, an exponent
    is appended to the significand as follows: If `value` is of
    type integer, the significand is kept as short as possible;
    otherwise (when `value` is of type real), it is written in
    scientific notation. In both cases, the exponent comprises at
    least two digits with an explicit sign. If `ebase` is absent,
    no exponent appears. The result is exact as long as `base` is a
    power of two.

  * Returns a string converted from `value`.

  * Throws an exception if `base` is neither `2` nor `10` nor `16`,
    or if `ebase` is neither `2` nor `10`, or if `base` is not `10`
    but `ebase` is `10`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.format"), ::rocket::cref(args));
    // Parse arguments.
    V_integer ivalue;
    Opt_integer base;
    Opt_integer ebase;
    if(reader.I().v(ivalue).o(base).o(ebase).F()) {
      Reference::S_temporary xref = { std_numeric_format(::std::move(ivalue), ::std::move(base),
                                                              ::std::move(ebase)) };
      return self = ::std::move(xref);
    }
    V_real fvalue;
    if(reader.I().v(fvalue).o(base).o(ebase).F()) {
      Reference::S_temporary xref = { std_numeric_format(::std::move(fvalue), ::std::move(base),
                                                              ::std::move(ebase)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.parse_integer()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("parse_integer"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.parse_integer(text)`

  * Parses `text` for an integer. `text` shall be a string. All
    leading and trailing blank characters are stripped from `text`.
    If it becomes empty, this function fails; otherwise, it shall
    match one of the following extended regular expressions:

    * Binary (base-2):
      `[+-]?0[bB][01]+`
    * Hexadecimal (base-16):
      `[+-]?0[xX][0-9a-fA-F]+`
    * Decimal (base-10):
      `[+-]?[0-9]+`

    If the string does not match any of the above, this function
    fails. If the result is outside the range of representable
    values of type integer, this function fails.

  * Returns the integer value converted from `text`.

  * Throws an exception on failure.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.parse_integer"), ::rocket::cref(args));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference::S_temporary xref = { std_numeric_parse_integer(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.numeric.parse_real()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("parse_real"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.parse_real(text, [saturating])`

  * Parses `text` for a real number. `text` shall be a string. All
    leading and trailing blank characters are stripped from `text`.
    If it becomes empty, this function fails; otherwise, it shall
    match any of the following extended regular expressions:

    * Infinities:
      `[+-]?infinity`
    * NaNs:
      `[+-]?nan`
    * Binary (base-2):
      `[+-]?0[bB][01]+(\.[01]+)?([epEP][-+]?[0-9]+)?`
    * Hexadecimal (base-16):
      `[+-]?0x[0-9a-fA-F]+(\.[0-9a-fA-F]+)?([epEP][-+]?[0-9]+)?`
    * Decimal (base-10):
      `[+-]?[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)?`

    If the string does not match any of the above, this function
    fails. If the absolute value of the result is too small to fit
    in a real, a signed zero is returned. When the absolute value
    is too large, if `saturating` is set to `true`, a signed
    infinity is returned; otherwise this function fails.

  * Returns the real value converted from `text`.

  * Throws an exception on failure.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, Global_Context& /*global*/, cow_vector<Reference>&& args) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.numeric.parse_real"), ::rocket::cref(args));
    // Parse arguments.
    V_string text;
    Opt_boolean saturating;
    if(reader.I().v(text).o(saturating).F()) {
      Reference::S_temporary xref = { std_numeric_parse_real(::std::move(text), saturating) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));
  }

}  // namespace asteria
