// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "numeric.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/random_engine.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

int64_t
do_verify_bounds(int64_t lower, int64_t upper)
  {
    if(!(lower <= upper)) {
      ASTERIA_THROW("Bounds not valid (`$1` is not less than or equal to `$2`)",
                    lower, upper);
    }
    return upper;
  }

double
do_verify_bounds(double lower, double upper)
  {
    if(!::std::islessequal(lower, upper)) {
      ASTERIA_THROW("Bounds not valid (`$1` is not less than or equal to `$2`)",
                    lower, upper);
    }
    return upper;
  }

int64_t
do_cast_to_integer(double value)
  {
    if(!is_convertible_to_integer(value)) {
      ASTERIA_THROW("Real value not representable as integer (value `$1`)",
                    value);
    }
    return static_cast<int64_t>(value);
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
      ASTERIA_THROW("Integer absolute value overflow (value `$1`)",
                    value);
    return ::std::abs(value);
  }

V_real
std_numeric_abs(V_real value)
  {
    return ::std::abs(value);
  }

V_integer
std_numeric_sign(V_integer value)
  {
    return value >> 63;
  }

V_integer
std_numeric_sign(V_real value)
  {
    return ::std::signbit(value) ? INT64_C(-1) : 0;
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

Value
std_numeric_max(cow_vector<Value> values)
  {
    Value res;
    for(const auto& val : values) {
      if(val.is_null())
        continue;

      if(!res.is_null()) {
        auto cmp = res.compare(val);
        if(cmp == compare_unordered)
          ASTERIA_THROW("Values not comparable (operands were `$1` and `$2`)",
                        cmp, val);

        if(cmp != compare_less)
          continue;
      }
      res = val;
    }
    return res;
  }

Value
std_numeric_min(cow_vector<Value> values)
  {
    Value res;
    for(const auto& val : values) {
      if(val.is_null())
        continue;

      if(!res.is_null()) {
        auto cmp = res.compare(val);
        if(cmp == compare_unordered)
          ASTERIA_THROW("Values not comparable (operands were `$1` and `$2`)",
                        cmp, val);

        if(cmp != compare_greater)
          continue;
      }
      res = val;
    }
    return res;
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
          ASTERIA_THROW("Random number limit shall be finite (limit `$1`)",
                        *limit);

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
    return ::std::ldexp(frac, ::rocket::clamp_cast<int>(exp, INT_MIN, INT_MAX));
  }

V_integer
std_numeric_addm(V_integer x, V_integer y)
  {
    return static_cast<int64_t>(static_cast<uint64_t>(x) + static_cast<uint64_t>(y));
  }

V_integer
std_numeric_subm(V_integer x, V_integer y)
  {
    return static_cast<int64_t>(static_cast<uint64_t>(x) - static_cast<uint64_t>(y));
  }

V_integer
std_numeric_mulm(V_integer x, V_integer y)
  {
    return static_cast<int64_t>(static_cast<uint64_t>(x) * static_cast<uint64_t>(y));
  }

V_integer
std_numeric_adds(V_integer x, V_integer y)
  {
    if((y >= 0) && (x > INT64_MAX - y))
      return INT64_MAX;

    if((y <= 0) && (x < INT64_MIN - y))
      return INT64_MIN;

    return x + y;
  }

V_real
std_numeric_adds(V_real x, V_real y)
  {
    return x + y;
  }

V_integer
std_numeric_subs(V_integer x, V_integer y)
  {
    if((y >= 0) && (x < INT64_MIN + y))
      return INT64_MIN;

    if((y <= 0) && (x > INT64_MAX + y))
      return INT64_MAX;

    return x - y;
  }

V_real
std_numeric_subs(V_real x, V_real y)
  {
    return x - y;
  }

V_integer
std_numeric_muls(V_integer x, V_integer y)
  {
    if((x == 0) || (y == 0))
      return 0;

    if((x == INT64_MIN) || (y == INT64_MIN))
      return ((x ^ y) >> 63) ^ INT64_MAX;

    int64_t m = y >> 63;
    int64_t s = (x ^ m) - m;  // x
    int64_t u = (y ^ m) - m;  // abs(y)

    if((s >= 0) && (s > INT64_MAX / u))
      return INT64_MAX;

    if((s <= 0) && (s < INT64_MIN / u))
      return INT64_MIN;

    return x * y;
  }

V_real
std_numeric_muls(V_real x, V_real y)
  {
    return x * y;
  }

V_integer
std_numeric_lzcnt(V_integer x)
  {
    if(ROCKET_UNEXPECT(x == 0))
      return 64;

    return ROCKET_LZCNT64_NZ(static_cast<uint64_t>(x));
  }

V_integer
std_numeric_tzcnt(V_integer x)
  {
    if(ROCKET_UNEXPECT(x == 0))
      return 64;

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
    result.insert_or_assign(sref("integer_max"),
      V_integer(
        ::std::numeric_limits<V_integer>::max()
      ));

    result.insert_or_assign(sref("integer_min"),
      V_integer(
        ::std::numeric_limits<V_integer>::lowest()
      ));

    result.insert_or_assign(sref("real_max"),
      V_real(
        ::std::numeric_limits<V_real>::max()
      ));

    result.insert_or_assign(sref("real_min"),
      V_real(
        ::std::numeric_limits<V_real>::lowest()
      ));

    result.insert_or_assign(sref("real_epsilon"),
      V_real(
        ::std::numeric_limits<V_real>::epsilon()
      ));

    result.insert_or_assign(sref("size_max"),
      V_integer(
        ::std::numeric_limits<ptrdiff_t>::max()
      ));

    result.insert_or_assign(sref("abs"),
      ASTERIA_BINDING_BEGIN("std.numeric.abs", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_abs, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_abs, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("sign"),
      ASTERIA_BINDING_BEGIN("std.numeric.sign", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_sign, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_sign, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("is_finite"),
      ASTERIA_BINDING_BEGIN("std.numeric.is_finite", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_is_finite, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_is_finite, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("is_infinity"),
      ASTERIA_BINDING_BEGIN("std.numeric.is_infinity", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_is_infinity, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_is_infinity, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("is_nan"),
      ASTERIA_BINDING_BEGIN("std.numeric.is_nan", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_is_nan, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_is_nan, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("max"),
      ASTERIA_BINDING_BEGIN("std.numeric.max", self, global, reader) {
        cow_vector<Value> vals;

        reader.start_overload();
        if(reader.end_overload(vals))   // ...
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_max, vals);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("min"),
      ASTERIA_BINDING_BEGIN("std.numeric.min", self, global, reader) {
        cow_vector<Value> vals;

        reader.start_overload();
        if(reader.end_overload(vals))   // ...
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_min, vals);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("clamp"),
      ASTERIA_BINDING_BEGIN("std.numeric.clamp", self, global, reader) {
        V_integer ival;
        V_integer ilo;
        V_integer iup;
        V_real fval;
        V_real flo;
        V_real fup;

        reader.start_overload();
        reader.required(ival);     // value
        reader.required(ilo);      // lower
        reader.required(iup);      // upper
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_clamp, ival, ilo, iup);

        reader.start_overload();
        reader.required(fval);     // value
        reader.required(flo);      // lower
        reader.required(fup);      // upper
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_clamp, fval, flo, fup);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("round"),
      ASTERIA_BINDING_BEGIN("std.numeric.round", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_round, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_round, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("iround"),
      ASTERIA_BINDING_BEGIN("std.numeric.iround", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_iround, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_iround, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("floor"),
      ASTERIA_BINDING_BEGIN("std.numeric.floor", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_floor, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_floor, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("ifloor"),
      ASTERIA_BINDING_BEGIN("std.numeric.ifloor", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_ifloor, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_ifloor, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("ceil"),
      ASTERIA_BINDING_BEGIN("std.numeric.ceil", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_ceil, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_ceil, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("iceil"),
      ASTERIA_BINDING_BEGIN("std.numeric.iceil", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_iceil, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_iceil, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("trunc"),
      ASTERIA_BINDING_BEGIN("std.numeric.trunc", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_trunc, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_trunc, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("itrunc"),
      ASTERIA_BINDING_BEGIN("std.numeric.itrunc", self, global, reader) {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_itrunc, ival);

        reader.start_overload();
        reader.required(fval);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_itrunc, fval);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("random"),
      ASTERIA_BINDING_BEGIN("std.numeric.random", self, global, reader) {
        Opt_real lim;

        reader.start_overload();
        reader.optional(lim);     // [limit]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_random, global, lim);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("sqrt"),
      ASTERIA_BINDING_BEGIN("std.numeric.sqrt", self, global, reader) {
        V_real val;

        reader.start_overload();
        reader.required(val);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_sqrt, val);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("fma"),
      ASTERIA_BINDING_BEGIN("std.numeric.fma", self, global, reader) {
        V_real x;
        V_real y;
        V_real z;

        reader.start_overload();
        reader.required(x);     // x
        reader.required(y);     // y
        reader.required(z);     // z
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_fma, x, y, z);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("remainder"),
      ASTERIA_BINDING_BEGIN("std.numeric.remainder", self, global, reader) {
        V_real x;
        V_real y;

        reader.start_overload();
        reader.required(x);     // x
        reader.required(y);     // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_remainder, x, y);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("frexp"),
      ASTERIA_BINDING_BEGIN("std.numeric.frexp", self, global, reader) {
        V_real val;

        reader.start_overload();
        reader.required(val);     // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_frexp, val);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("ldexp"),
      ASTERIA_BINDING_BEGIN("std.numeric.ldexp", self, global, reader) {
        V_real frac;
        V_integer exp;

        reader.start_overload();
        reader.required(frac);    // frac
        reader.required(exp);     // exp
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_ldexp, frac, exp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("addm"),
      ASTERIA_BINDING_BEGIN("std.numeric.addm", self, global, reader) {
        V_integer x;
        V_integer y;

        reader.start_overload();
        reader.required(x);    // x
        reader.required(y);    // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_addm, x, y);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("subm"),
      ASTERIA_BINDING_BEGIN("std.numeric.subm", self, global, reader) {
        V_integer x;
        V_integer y;

        reader.start_overload();
        reader.required(x);    // x
        reader.required(y);    // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_subm, x, y);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("mulm"),
      ASTERIA_BINDING_BEGIN("std.numeric.mulm", self, global, reader) {
        V_integer x;
        V_integer y;

        reader.start_overload();
        reader.required(x);    // x
        reader.required(y);    // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_mulm, x, y);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("adds"),
      ASTERIA_BINDING_BEGIN("std.numeric.adds", self, global, reader) {
        V_integer ix;
        V_integer iy;
        V_real fx;
        V_real fy;

        reader.start_overload();
        reader.required(ix);    // x
        reader.required(iy);    // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_adds, ix, iy);

        reader.start_overload();
        reader.required(fx);    // x
        reader.required(fy);    // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_adds, fx, fy);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("subs"),
      ASTERIA_BINDING_BEGIN("std.numeric.subs", self, global, reader) {
        V_integer ix;
        V_integer iy;
        V_real fx;
        V_real fy;

        reader.start_overload();
        reader.required(ix);    // x
        reader.required(iy);    // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_subs, ix, iy);

        reader.start_overload();
        reader.required(fx);    // x
        reader.required(fy);    // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_subs, fx, fy);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("muls"),
      ASTERIA_BINDING_BEGIN("std.numeric.muls", self, global, reader) {
        V_integer ix;
        V_integer iy;
        V_real fx;
        V_real fy;

        reader.start_overload();
        reader.required(ix);    // x
        reader.required(iy);    // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_muls, ix, iy);

        reader.start_overload();
        reader.required(fx);    // x
        reader.required(fy);    // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_muls, fx, fy);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("lzcnt"),
      ASTERIA_BINDING_BEGIN("std.numeric.lzcnt", self, global, reader) {
        V_integer x;

        reader.start_overload();
        reader.required(x);    // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_lzcnt, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("tzcnt"),
      ASTERIA_BINDING_BEGIN("std.numeric.tzcnt", self, global, reader) {
        V_integer x;

        reader.start_overload();
        reader.required(x);    // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_tzcnt, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("popcnt"),
      ASTERIA_BINDING_BEGIN("std.numeric.popcnt", self, global, reader) {
        V_integer x;

        reader.start_overload();
        reader.required(x);    // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_popcnt, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("rotl"),
      ASTERIA_BINDING_BEGIN("std.numeric.rotl", self, global, reader) {
        V_integer m;
        V_integer x;
        V_integer sh;

        reader.start_overload();
        reader.required(m);    // m
        reader.required(x);    // x
        reader.required(sh);   // shift
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_rotl, m, x, sh);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("rotr"),
      ASTERIA_BINDING_BEGIN("std.numeric.rotr", self, global, reader) {
        V_integer m;
        V_integer x;
        V_integer sh;

        reader.start_overload();
        reader.required(m);    // m
        reader.required(x);    // x
        reader.required(sh);   // shift
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_rotr, m, x, sh);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("format"),
      ASTERIA_BINDING_BEGIN("std.numeric.format", self, global, reader) {
        V_integer ival;
        V_real fval;
        Opt_integer base;
        Opt_integer ebase;

        reader.start_overload();
        reader.required(ival);    // value
        reader.optional(base);    // [base]
        reader.optional(ebase);   // [ebase]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_format, ival, base, ebase);

        reader.start_overload();
        reader.required(fval);    // value
        reader.optional(base);    // [base]
        reader.optional(ebase);   // [ebase]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_format, fval, base, ebase);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("parse_integer"),
      ASTERIA_BINDING_BEGIN("std.numeric.parse_integer", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_parse_integer, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("parse_real"),
      ASTERIA_BINDING_BEGIN("std.numeric.parse_real", self, global, reader) {
        V_string text;
        Opt_boolean satur;

        reader.start_overload();
        reader.required(text);    // text
        reader.optional(satur);   // [saturating]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_numeric_parse_real, text, satur);
      }
      ASTERIA_BINDING_END);
  }

}  // namespace asteria
