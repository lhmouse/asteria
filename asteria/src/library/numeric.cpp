// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "numeric.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/random_engine.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

int64_t do_verify_bounds(int64_t lower, int64_t upper)
  {
    if(!(lower <= upper)) {
      ASTERIA_THROW("bounds not valid (`$1` is not less than or equal to `$2`)", lower, upper);
    }
    return upper;
  }

double do_verify_bounds(double lower, double upper)
  {
    if(!::std::islessequal(lower, upper)) {
      ASTERIA_THROW("bounds not valid (`$1` is not less than or equal to `$2`)", lower, upper);
    }
    return upper;
  }

Ival do_cast_to_integer(double value)
  {
    if(!::std::islessequal(-0x1p63, value) || !::std::islessequal(value, 0x1p63 - 0x1p10)) {
      ASTERIA_THROW("`real` value not representable as an `integer` (value `$1`)", value);
    }
    return Ival(value);
  }

ROCKET_PURE_FUNCTION Ival do_saturating_add(int64_t lhs, int64_t rhs)
  {
    if((rhs >= 0) ? (lhs > INT64_MAX - rhs) : (lhs < INT64_MIN - rhs)) {
      return (rhs >> 63) ^ INT64_MAX;
    }
    return lhs + rhs;
  }

ROCKET_PURE_FUNCTION Ival do_saturating_sub(int64_t lhs, int64_t rhs)
  {
    if((rhs >= 0) ? (lhs < INT64_MIN + rhs) : (lhs > INT64_MAX + rhs)) {
      return (rhs >> 63) ^ INT64_MIN;
    }
    return lhs - rhs;
  }

ROCKET_PURE_FUNCTION Ival do_saturating_mul(int64_t lhs, int64_t rhs)
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

ROCKET_PURE_FUNCTION Rval do_saturating_add(double lhs, double rhs)
  { return lhs + rhs;  }

ROCKET_PURE_FUNCTION Rval do_saturating_sub(double lhs, double rhs)
  { return lhs - rhs;  }

ROCKET_PURE_FUNCTION Rval do_saturating_mul(double lhs, double rhs)
  { return lhs * rhs;  }

pair<Ival, int> do_decompose_integer(uint8_t ebase, int64_t value)
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
    auto prng = global.random_engine();

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

void create_bindings_numeric(V_object& result, API_Version /*version*/)
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.abs"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_abs(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_abs(::std::move(rvalue)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.sign(value)`

  * Propagates the sign bit of the number `value`, which may be an
    integer or real, to all bits of an integer. Be advised that
    `-0.0` is distinct from `0.0` despite the equality.

  * Returns `-1` if `value` is negative, or `0` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.sign"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_sign(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_sign(::std::move(rvalue)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.is_finite(value)`

  * Checks whether `value` is a finite number. `value` may be an
    integer or real. Be adviced that this functions returns `true`
    for integers for consistency; integers do not support
    infinities or NaNs.

  * Returns `true` if `value` is an integer or is a real that
    is neither an infinity or a NaN, or `false` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.is_finite"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_is_finite(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_is_finite(::std::move(rvalue)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.is_infinity(value)`

  * Checks whether `value` is an infinity. `value` may be an
    integer or real. Be adviced that this functions returns `false`
    for integers for consistency; integers do not support
    infinities.

  * Returns `true` if `value` is a real that denotes an infinity;
    or `false` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.is_infinity"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_is_infinity(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_is_infinity(::std::move(rvalue)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.is_nan(value)`

  * Checks whether `value` is a NaN. `value` may be an integer or
    real. Be adviced that this functions returns `false` for
    integers for consistency; integers do not support NaNs.

  * Returns `true` if `value` is a real denoting a NaN, or
    `false` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.is_nan"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_is_nan(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_is_nan(::std::move(rvalue)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.clamp"));
    // Parse arguments.
    Ival ivalue;
    Ival ilower;
    Ival iupper;
    if(reader.I().v(ivalue).v(ilower).v(iupper).F()) {
      Reference_root::S_temporary xref = { std_numeric_clamp(::std::move(ivalue), ::std::move(ilower), ::std::move(iupper)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    Rval flower;
    Rval fupper;
    if(reader.I().v(rvalue).v(flower).v(fupper).F()) {
      Reference_root::S_temporary xref = { std_numeric_clamp(::std::move(rvalue), ::std::move(flower), ::std::move(fupper)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.round(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer; halfway values are rounded away from zero. If `value`
    is an integer, it is returned intact.

  * Returns the rounded value.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.round"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_round(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_round(::std::move(rvalue)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.floor(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer towards negative infinity. If `value` is an integer, it
    is returned intact.

  * Returns the rounded value.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.floor"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_floor(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_floor(::std::move(rvalue)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.ceil(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer towards positive infinity. If `value` is an integer,
    it is returned intact.

  * Returns the rounded value.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.ceil"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_ceil(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_ceil(::std::move(rvalue)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.trunc(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer towards zero. If `value` is an integer, it is returned
    intact.

  * Returns the rounded value.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.trunc"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_trunc(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_trunc(::std::move(rvalue)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.iround"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_iround(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_iround(::std::move(rvalue)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.ifloor"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_ifloor(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_ifloor(::std::move(rvalue)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.iceil"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_iceil(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_iceil(::std::move(rvalue)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.itrunc(value)`

  * Rounds `value`, which may be an integer or real, to the nearest
    integer towards zero. If `value` is an integer, it is returned
    intact. If `value` is a real, it is converted to an integer.

  * Returns the rounded value as an integer.

  * Throws an exception if the result cannot be represented as an
    integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.itrunc"));
    // Parse arguments.
    Ival ivalue;
    if(reader.I().v(ivalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_itrunc(::std::move(ivalue)) };
      return self = ::std::move(xref);
    }
    Rval rvalue;
    if(reader.I().v(rvalue).F()) {
      Reference_root::S_temporary xref = { std_numeric_itrunc(::std::move(rvalue)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.random([limit])`

  * Generates a random real value whose sign agrees with `limit`
    and whose absolute value is less than `limit`. If `limit` is
    absent, `1` is assumed.

  * Returns a random real value.

  * Throws an exception if `limit` is zero or non-finite.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& global) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.random"));
    // Parse arguments.
    Ropt limit;
    if(reader.I().o(limit).F()) {
      Reference_root::S_temporary xref = { std_numeric_random(global, ::std::move(limit)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.sqrt(x)`

  * Calculates the square root of `x` which may be of either the
    integer or the real type. The result is always a real.

  * Returns the square root of `x` as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.sqrt"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_numeric_sqrt(::std::move(x)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.fma(x, y, z)`

  * Performs fused multiply-add operation on `x`, `y` and `z`. This
    functions calculates `x * y + z` without intermediate rounding
    operations.

  * Returns the value of `x * y + z` as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.fma"));
    // Parse arguments.
    Rval x;
    Rval y;
    Rval z;
    if(reader.I().v(x).v(y).v(z).F()) {
      Reference_root::S_temporary xref = { std_numeric_fma(::std::move(x), ::std::move(y), ::std::move(z)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.remainder(x, y)`

  * Calculates the IEEE floating-point remainder of division of `x`
    by `y`. The remainder is defined to be `x - q * y` where `q` is
    the quotient of division of `x` by `y` rounding to nearest.

  * Returns the remainder as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.remainder"));
    // Parse arguments.
    Rval x;
    Rval y;
    if(reader.I().v(x).v(y).F()) {
      Reference_root::S_temporary xref = { std_numeric_remainder(::std::move(x), ::std::move(y)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.frexp"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      auto pair = std_numeric_frexp(::std::move(x));
      // This function returns a `pair`, but we would like to return an array so convert it.
      Reference_root::S_temporary xref = { { pair.first, pair.second } };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.ldexp(frac, exp)`

  * Composes `frac` and `exp` to make a real number `x`, as if by
    multiplying `frac` with `pow(2,exp)`. `exp` shall be of type
    integer. This function is the inverse of `frexp()`.

  * Returns the product as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.ldexp"));
    // Parse arguments.
    Rval frac;
    Ival exp;
    if(reader.I().v(frac).v(exp).F()) {
      Reference_root::S_temporary xref = { std_numeric_ldexp(::std::move(frac), ::std::move(exp)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.addm(x, y)`

  * Adds `y` to `x` using modular arithmetic. `x` and `y` must be
    of the integer type. The result is reduced to be congruent to
    the sum of `x` and `y` modulo `0x1p64` in infinite precision.
    This function will not cause overflow exceptions to be thrown.

  * Returns the reduced sum of `x` and `y`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.addm"));
    // Parse arguments.
    Ival x;
    Ival y;
    if(reader.I().v(x).v(y).F()) {
      Reference_root::S_temporary xref = { std_numeric_addm(::std::move(x), ::std::move(y)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.subm(x, y)`

  * Subtracts `y` from `x` using modular arithmetic. `x` and `y`
    must be of the integer type. The result is reduced to be
    congruent to the difference of `x` and `y` modulo `0x1p64` in
    infinite precision. This function will not cause overflow
    exceptions to be thrown.

  * Returns the reduced difference of `x` and `y`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.subm"));
    // Parse arguments.
    Ival x;
    Ival y;
    if(reader.I().v(x).v(y).F()) {
      Reference_root::S_temporary xref = { std_numeric_subm(::std::move(x), ::std::move(y)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.mulm(x, y)`

  * Multiplies `x` by `y` using modular arithmetic. `x` and `y`
    must be of the integer type. The result is reduced to be
    congruent to the product of `x` and `y` modulo `0x1p64` in
    infinite precision. This function will not cause overflow
    exceptions to be thrown.

  * Returns the reduced product of `x` and `y`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.mulm"));
    // Parse arguments.
    Ival x;
    Ival y;
    if(reader.I().v(x).v(y).F()) {
      Reference_root::S_temporary xref = { std_numeric_mulm(::std::move(x), ::std::move(y)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.adds"));
    // Parse arguments.
    Ival ix;
    Ival iy;
    if(reader.I().v(ix).v(iy).F()) {
      Reference_root::S_temporary xref = { std_numeric_adds(::std::move(ix), ::std::move(iy)) };
      return self = ::std::move(xref);
    }
    Rval fx;
    Rval fy;
    if(reader.I().v(fx).v(fy).F()) {
      Reference_root::S_temporary xref = { std_numeric_adds(::std::move(fx), ::std::move(fy)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.subs"));
    // Parse arguments.
    Ival ix;
    Ival iy;
    if(reader.I().v(ix).v(iy).F()) {
      Reference_root::S_temporary xref = { std_numeric_subs(::std::move(ix), ::std::move(iy)) };
      return self = ::std::move(xref);
    }
    Rval fx;
    Rval fy;
    if(reader.I().v(fx).v(fy).F()) {
      Reference_root::S_temporary xref = { std_numeric_subs(::std::move(fx), ::std::move(fy)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.muls"));
    // Parse arguments.
    Ival ix;
    Ival iy;
    if(reader.I().v(ix).v(iy).F()) {
      Reference_root::S_temporary xref = { std_numeric_muls(::std::move(ix), ::std::move(iy)) };
      return self = ::std::move(xref);
    }
    Rval fx;
    Rval fy;
    if(reader.I().v(fx).v(fy).F()) {
      Reference_root::S_temporary xref = { std_numeric_muls(::std::move(fx), ::std::move(fy)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.lzcnt(x)`

  * Counts the number of leading zero bits in `x`, which shall be
    of type integer.

  * Returns the bit count as an integer. If `x` is zero, `64` is
    returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.lzcnt"));
    // Parse arguments.
    Ival x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_numeric_lzcnt(::std::move(x)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.tzcnt(x)`

  * Counts the number of trailing zero bits in `x`, which shall be
    of type integer.

  * Returns the bit count as an integer. If `x` is zero, `64` is
    returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.tzcnt"));
    // Parse arguments.
    Ival x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_numeric_tzcnt(::std::move(x)) };
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
      Fval(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.numeric.popcnt(x)`

  * Counts the number of one bits in `x`, which shall be of type
    integer.

  * Returns the bit count as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.popcnt"));
    // Parse arguments.
    Ival x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_numeric_popcnt(::std::move(x)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.rotl"));
    // Parse arguments.
    Ival m;
    Ival x;
    Ival n;
    if(reader.I().v(m).v(x).v(n).F()) {
      Reference_root::S_temporary xref = { std_numeric_rotl(::std::move(m), ::std::move(x), ::std::move(n)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.rotr"));
    // Parse arguments.
    Ival m;
    Ival x;
    Ival n;
    if(reader.I().v(m).v(x).v(n).F()) {
      Reference_root::S_temporary xref = { std_numeric_rotr(::std::move(m), ::std::move(x), ::std::move(n)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.format"));
    // Parse arguments.
    Ival ivalue;
    Iopt base;
    Iopt ebase;
    if(reader.I().v(ivalue).o(base).o(ebase).F()) {
      Reference_root::S_temporary xref = { std_numeric_format(::std::move(ivalue), ::std::move(base), ::std::move(ebase)) };
      return self = ::std::move(xref);
    }
    Rval fvalue;
    if(reader.I().v(fvalue).o(base).o(ebase).F()) {
      Reference_root::S_temporary xref = { std_numeric_format(::std::move(fvalue), ::std::move(base), ::std::move(ebase)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.parse_integer"));
    // Parse arguments.
    Sval text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_numeric_parse_integer(::std::move(text)) };
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
      Fval(
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
*[](Reference& self, cow_vector<Reference>&& args, Global& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.numeric.parse_real"));
    // Parse arguments.
    Sval text;
    Bopt saturating;
    if(reader.I().v(text).o(saturating).F()) {
      Reference_root::S_temporary xref = { std_numeric_parse_real(::std::move(text), ::std::move(saturating)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));
    //===================================================================
    // End of `std.numeric`
    //===================================================================
  }

}  // namespace Asteria
