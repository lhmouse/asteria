// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "numeric.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/random_engine.hpp"
#include "../utils.hpp"
namespace asteria {
namespace {

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

    // Format the integer.
    // If the exponent is negative, a minus sign will have been added.
    nump.put_DI(exp, 2);

    // Append significant figures.
    text.append(nump.begin(), nump.end());
    return text;
  }

template<typename objectT, typename sourceT>
ROCKET_ALWAYS_INLINE
objectT
do_bit_cast(const sourceT& src)
  noexcept
  {
    static_assert(sizeof(objectT) == sizeof(sourceT), "size mismatch");
    objectT dst;
    ::memcpy(&dst, &src, sizeof(objectT));
    return dst;
  }

}  // namespace

V_integer
std_numeric_abs(V_integer value)
  {
    if(value == INT64_MIN)
      ASTERIA_THROW((
          "Integer absolute value overflow (value `$1`)"),
          value);

    return ::std::abs(value);
  }

V_real
std_numeric_abs(V_real value)
  {
    return ::std::abs(value);
  }

V_boolean
std_numeric_sign(V_integer value)
  {
    return value < 0;
  }

V_boolean
std_numeric_sign(V_real value)
  {
    return ::std::signbit(value);
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
    Value result;
    for(const auto& r : values)
      if(result.is_null() || (result.compare_partial(r) == compare_less))
        result = r;
    return result;
  }

Value
std_numeric_min(cow_vector<Value> values)
  {
    Value result;
    for(const auto& r : values)
      if(result.is_null() || (result.compare_partial(r) == compare_greater))
        result = r;
    return result;
  }

Value
std_numeric_clamp(Value value, Value lower, Value upper)
  {
    if(value.compare_total(lower) == compare_less)
      return lower;

    if(value.compare_total(upper) == compare_greater)
      return upper;

    return value;
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
    return safe_double_to_int64(::std::round(value));
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
    return safe_double_to_int64(::std::floor(value));
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
    return safe_double_to_int64(::std::ceil(value));
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
    return safe_double_to_int64(::std::trunc(value));
  }

V_real
std_numeric_random(Global_Context& global, optV_real limit)
  {
    // Generate a random `double` in the range [0.0,1.0).
    const auto prng = global.random_engine();
    int64_t ireg = prng->bump();
    ireg <<= 21;
    ireg ^= prng->bump();
    double ratio = static_cast<double>(ireg) * 0x1p-53;

    // If a limit is specified, magnify the value.
    // The default magnitude is 1.0 so no action is taken.
    if(limit)
      switch(::std::fpclassify(*limit))
        {
        case FP_ZERO:
          ASTERIA_THROW(("Random number limit was zero"));

        case FP_INFINITE:
        case FP_NAN:
          ASTERIA_THROW(("Random number limit `$1` was not finite"), *limit);

        default:
          ratio *= *limit;
      }

    return ratio;
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
std_numeric_rotl(V_integer m, V_integer x, V_integer n)
  {
    if((m < 0) || (m > 64))
      ASTERIA_THROW((
          "Invalid modulo bit count (`$1` is not between 0 and 64)"), m);

    if(m == 0)
      return 0;

    // The shift count is modulo `m` so all values are defined.
    uint64_t ireg = static_cast<uint64_t>(x);
    uint64_t mask = (2ULL << (m - 1)) - 1;
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
      ASTERIA_THROW((
          "Invalid modulo bit count (`$1` is not between 0 and 64)"), m);

    if(m == 0)
      return 0;

    // The shift count is modulo `m` so all values are defined.
    uint64_t ireg = static_cast<uint64_t>(x);
    uint64_t mask = (2ULL << (m - 1)) - 1;
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
std_numeric_format(V_integer value, optV_integer base, optV_integer ebase)
  {
    V_string text;
    ::rocket::ascii_numput nump;

    switch(base.value_or(10))
      {
      case 2:
        {
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

          ASTERIA_THROW((
              "Invalid exponent base for binary notation (`$1` is not 2)"),
              *ebase);
        }

      case 16:
        {
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

          ASTERIA_THROW((
              "Invalid exponent base for hexadecimal notation (`$1` is not 2)"),
              *ebase);
        }

      case 10:
        {
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

          ASTERIA_THROW((
              "Invalid exponent base for decimal notation (`$1` is not 10)"),
              *ebase);
        }

      default:
        ASTERIA_THROW((
            "Invalid number base (base `$1` is not one of { 2, 10, 16 })"),
            *base);
    }
    return text;
  }

V_string
std_numeric_format(V_real value, optV_integer base, optV_integer ebase)
  {
    V_string text;
    ::rocket::ascii_numput nump;

    switch(base.value_or(10))
      {
      case 2:
        {
          if(!ebase) {
            nump.put_BD(value);  // binary, float
            text.append(nump.begin(), nump.end());
            break;
          }

          if(*ebase == 2) {
            nump.put_BED(value);  // binary, scientific
            text.append(nump.begin(), nump.end());
            break;
          }

          ASTERIA_THROW((
              "Invalid exponent base for binary notation (`$1` is not 2)"),
              *ebase);
        }

      case 16:
        {
          if(!ebase) {
            nump.put_XD(value);  // hexadecimal, float
            text.append(nump.begin(), nump.end());
            break;
          }

          if(*ebase == 2) {
            nump.put_XED(value);  // hexadecimal, scientific
            text.append(nump.begin(), nump.end());
            break;
          }

          ASTERIA_THROW((
              "Invalid exponent base for hexadecimal notation (`$1` is not 2)"),
              *ebase);
        }

      case 10:
        {
          if(!ebase) {
            nump.put_DD(value);  // decimal, float
            text.append(nump.begin(), nump.end());
            break;
          }

          if(*ebase == 10) {
            nump.put_DED(value);  // decimal, scientific
            text.append(nump.begin(), nump.end());
            break;
          }

          ASTERIA_THROW((
              "Invalid exponent base for decimal notation (`$1` is not 10)"),
              *ebase);
        }

      default:
        ASTERIA_THROW((
            "Invalid number base (base `$1` is not one of { 2, 10, 16 })"),
            *base);
    }
    return text;
  }

Value
std_numeric_parse(V_string text)
  {
    static constexpr char s_spaces[] = " \f\n\r\t\v";
    size_t tpos = text.find_not_of(s_spaces);
    if(tpos == V_string::npos)
      ASTERIA_THROW(("Blank string"));

    // Try parsing the string as a real number first.
    ::rocket::ascii_numget numg;
    size_t tlen = text.rfind_not_of(s_spaces) + 1 - tpos;
    if(numg.parse_D(text.data() + tpos, tlen) != tlen)
      ASTERIA_THROW((
          "String not convertible to a number (text was `$1`)"), text);

    if(text.find('.') == V_string::npos) {
      // Check whether the value fits in an integer. If so, return it exact.
      V_integer ival;
      numg.cast_I(ival, INT64_MIN, INT64_MAX);
      if(!numg.overflowed() && !numg.underflowed() && !numg.inexact())
        return ival;
    }

    // ... no, so return a (maybe approximate) real number.
    V_real fval;
    numg.cast_D(fval, -HUGE_VAL, HUGE_VAL);
    return fval;
  }

V_string
std_numeric_pack_i8(V_integer value)
  {
    return V_string(1, static_cast<char>(value));
  }

V_string
std_numeric_pack_i8(V_array values)
  {
    V_string text;
    text.reserve(values.size());
    for(const auto& r : values) {
      V_integer value = r.as_integer();
      text.push_back(static_cast<char>(value));
    }
    return text;
  }

V_array
std_numeric_unpack_i8(V_string text)
  {
    V_array values;
    values.reserve(text.size());
    for(size_t off = 0;  off != text.size();  off ++) {
      V_integer value = static_cast<int8_t>(*(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

V_string
std_numeric_pack_i16be(V_integer value)
  {
    char piece[2];
    ROCKET_STORE_BE16(piece, static_cast<uint16_t>(value));
    return V_string(piece, sizeof(piece));
  }

V_string
std_numeric_pack_i16be(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 2);
    for(const auto& r : values) {
      char piece[2];
      ROCKET_STORE_BE16(piece, static_cast<uint16_t>(r.as_integer()));
      text.append(piece, sizeof(piece));
    }
    return text;
  }

V_array
std_numeric_unpack_i16be(V_string text)
  {
    if(text.size() % 2 != 0)
      ASTERIA_THROW(("String length `$1` not divisible by 2"), text.size());

    V_array values;
    values.reserve(text.size() / 2);
    for(size_t off = 0;  off != text.size();  off += 2) {
      V_integer value = static_cast<int16_t>(ROCKET_LOAD_BE16(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

V_string
std_numeric_pack_i16le(V_integer value)
  {
    char piece[2];
    ROCKET_STORE_LE16(piece, static_cast<uint16_t>(value));
    return V_string(piece, sizeof(piece));
  }

V_string
std_numeric_pack_i16le(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 2);
    for(const auto& r : values) {
      char piece[2];
      ROCKET_STORE_LE16(piece, static_cast<uint16_t>(r.as_integer()));
      text.append(piece, sizeof(piece));
    }
    return text;
  }

V_array
std_numeric_unpack_i16le(V_string text)
  {
    if(text.size() % 2 != 0)
      ASTERIA_THROW(("String length `$1` not divisible by 2"), text.size());

    V_array values;
    values.reserve(text.size() / 2);
    for(size_t off = 0;  off != text.size();  off += 2) {
      V_integer value = static_cast<int16_t>(ROCKET_LOAD_LE16(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

V_string
std_numeric_pack_i32be(V_integer value)
  {
    char piece[4];
    ROCKET_STORE_BE32(piece, static_cast<uint32_t>(value));
    return V_string(piece, sizeof(piece));
  }

V_string
std_numeric_pack_i32be(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 4);
    for(const auto& r : values) {
      char piece[4];
      ROCKET_STORE_BE32(piece, static_cast<uint32_t>(r.as_integer()));
      text.append(piece, sizeof(piece));
    }
    return text;
  }

V_array
std_numeric_unpack_i32be(V_string text)
  {
    if(text.size() % 4 != 0)
      ASTERIA_THROW(("String length `$1` not divisible by 4"), text.size());

    V_array values;
    values.reserve(text.size() / 4);
    for(size_t off = 0;  off != text.size();  off += 4) {
      V_integer value = static_cast<int32_t>(ROCKET_LOAD_BE32(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

V_string
std_numeric_pack_i32le(V_integer value)
  {
    char piece[4];
    ROCKET_STORE_LE32(piece, static_cast<uint32_t>(value));
    return V_string(piece, sizeof(piece));
  }

V_string
std_numeric_pack_i32le(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 4);
    for(const auto& r : values) {
      char piece[4];
      ROCKET_STORE_LE32(piece, static_cast<uint32_t>(r.as_integer()));
      text.append(piece, sizeof(piece));
    }
    return text;
  }

V_array
std_numeric_unpack_i32le(V_string text)
  {
    if(text.size() % 4 != 0)
      ASTERIA_THROW(("String length `$1` not divisible by 4"), text.size());

    V_array values;
    values.reserve(text.size() / 4);
    for(size_t off = 0;  off != text.size();  off += 4) {
      V_integer value = static_cast<int32_t>(ROCKET_LOAD_LE32(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

V_string
std_numeric_pack_i64be(V_integer value)
  {
    char piece[8];
    ROCKET_STORE_BE64(piece, static_cast<uint64_t>(value));
    return V_string(piece, sizeof(piece));
  }

V_string
std_numeric_pack_i64be(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 8);
    for(const auto& r : values) {
      char piece[8];
      ROCKET_STORE_BE64(piece, static_cast<uint64_t>(r.as_integer()));
      text.append(piece, sizeof(piece));
    }
    return text;
  }

V_array
std_numeric_unpack_i64be(V_string text)
  {
    if(text.size() % 8 != 0)
      ASTERIA_THROW(("String length `$1` not divisible by 8"), text.size());

    V_array values;
    values.reserve(text.size() / 8);
    for(size_t off = 0;  off != text.size();  off += 8) {
      V_integer value = static_cast<int64_t>(ROCKET_LOAD_BE64(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

V_string
std_numeric_pack_i64le(V_integer value)
  {
    char piece[8];
    ROCKET_STORE_LE64(piece, static_cast<uint64_t>(value));
    return V_string(piece, sizeof(piece));
  }

V_string
std_numeric_pack_i64le(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 8);
    for(const auto& r : values) {
      char piece[8];
      ROCKET_STORE_LE64(piece, static_cast<uint64_t>(r.as_integer()));
      text.append(piece, sizeof(piece));
    }
    return text;
  }

V_array
std_numeric_unpack_i64le(V_string text)
  {
    if(text.size() % 8 != 0)
      ASTERIA_THROW(("String length `$1` not divisible by 8"), text.size());

    V_array values;
    values.reserve(text.size() / 8);
    for(size_t off = 0;  off != text.size();  off += 8) {
      V_integer value = static_cast<int64_t>(ROCKET_LOAD_LE64(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

V_string
std_numeric_pack_f32be(V_real value)
  {
    char piece[4];
    ROCKET_STORE_BE32(piece, do_bit_cast<uint32_t>(static_cast<float>(value)));
    return V_string(piece, sizeof(piece));
  }

V_string
std_numeric_pack_f32be(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 4);
    for(const auto& r : values) {
      char piece[4];
      ROCKET_STORE_BE32(piece, do_bit_cast<uint32_t>(static_cast<float>(r.as_real())));
      text.append(piece, sizeof(piece));
    }
    return text;
  }

V_array
std_numeric_unpack_f32be(V_string text)
  {
    if(text.size() % 4 != 0)
      ASTERIA_THROW(("String length `$1` not divisible by 4"), text.size());

    V_array values;
    values.reserve(text.size() / 4);
    for(size_t off = 0;  off != text.size();  off += 4) {
      V_real value = do_bit_cast<float>(ROCKET_LOAD_BE32(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

V_string
std_numeric_pack_f32le(V_real value)
  {
    char piece[4];
    ROCKET_STORE_LE32(piece, do_bit_cast<uint32_t>(static_cast<float>(value)));
    return V_string(piece, sizeof(piece));
  }

V_string
std_numeric_pack_f32le(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 4);
    for(const auto& r : values) {
      char piece[4];
      ROCKET_STORE_LE32(piece, do_bit_cast<uint32_t>(static_cast<float>(r.as_real())));
      text.append(piece, sizeof(piece));
    }
    return text;
  }

V_array
std_numeric_unpack_f32le(V_string text)
  {
    if(text.size() % 4 != 0)
      ASTERIA_THROW(("String length `$1` not divisible by 4"), text.size());

    V_array values;
    values.reserve(text.size() / 4);
    for(size_t off = 0;  off != text.size();  off += 4) {
      V_real value = do_bit_cast<float>(ROCKET_LOAD_LE32(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

V_string
std_numeric_pack_f64be(V_real value)
  {
    char piece[8];
    ROCKET_STORE_BE64(piece, do_bit_cast<uint64_t>(static_cast<double>(value)));
    return V_string(piece, sizeof(piece));
  }

V_string
std_numeric_pack_f64be(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 8);
    for(const auto& r : values) {
      char piece[8];
      ROCKET_STORE_BE64(piece, do_bit_cast<uint64_t>(static_cast<double>(r.as_real())));
      text.append(piece, sizeof(piece));
    }
    return text;
  }

V_array
std_numeric_unpack_f64be(V_string text)
  {
    if(text.size() % 8 != 0)
      ASTERIA_THROW(("String length `$1` not divisible by 8"), text.size());

    V_array values;
    values.reserve(text.size() / 8);
    for(size_t off = 0;  off != text.size();  off += 8) {
      V_real value = do_bit_cast<double>(ROCKET_LOAD_BE64(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

V_string
std_numeric_pack_f64le(V_real value)
  {
    char piece[8];
    ROCKET_STORE_LE64(piece, do_bit_cast<uint64_t>(static_cast<double>(value)));
    return V_string(piece, sizeof(piece));
  }

V_string
std_numeric_pack_f64le(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 8);
    for(const auto& r : values) {
      char piece[8];
      ROCKET_STORE_LE64(piece, do_bit_cast<uint64_t>(static_cast<double>(r.as_real())));
      text.append(piece, sizeof(piece));
    }
    return text;
  }

V_array
std_numeric_unpack_f64le(V_string text)
  {
    if(text.size() % 8 != 0)
      ASTERIA_THROW(("String length `$1` not divisible by 8"), text.size());

    V_array values;
    values.reserve(text.size() / 8);
    for(size_t off = 0;  off != text.size();  off += 8) {
      V_real value = do_bit_cast<double>(ROCKET_LOAD_LE64(text.data() + off));
      values.emplace_back(value);
    }
    return values;
  }

void
create_bindings_numeric(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(&"integer_max",
      V_integer(
        ::std::numeric_limits<V_integer>::max()
      ));

    result.insert_or_assign(&"integer_min",
      V_integer(
        ::std::numeric_limits<V_integer>::lowest()
      ));

    result.insert_or_assign(&"real_max",
      V_real(
        ::std::numeric_limits<V_real>::max()
      ));

    result.insert_or_assign(&"real_min",
      V_real(
        ::std::numeric_limits<V_real>::lowest()
      ));

    result.insert_or_assign(&"real_epsilon",
      V_real(
        ::std::numeric_limits<V_real>::epsilon()
      ));

    result.insert_or_assign(&"size_max",
      V_integer(
        ::std::numeric_limits<ptrdiff_t>::max()
      ));

    result.insert_or_assign(&"abs",
      ASTERIA_BINDING(
        "std.numeric.abs", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_abs(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_abs(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"sign",
      ASTERIA_BINDING(
        "std.numeric.sign", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_sign(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_sign(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"is_finite",
      ASTERIA_BINDING(
        "std.numeric.is_finite", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_is_finite(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_is_finite(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"is_infinity",
      ASTERIA_BINDING(
        "std.numeric.is_finite", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_is_infinity(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_is_infinity(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"is_nan",
      ASTERIA_BINDING(
        "std.numeric.is_nan", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_is_nan(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_is_nan(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"max",
      ASTERIA_BINDING(
        "std.numeric.max", "...",
        Argument_Reader&& reader)
      {
        cow_vector<Value> vals;

        reader.start_overload();
        if(reader.end_overload(vals))
          return (Value) std_numeric_max(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"min",
      ASTERIA_BINDING(
        "std.numeric.min", "...",
        Argument_Reader&& reader)
      {
        cow_vector<Value> vals;

        reader.start_overload();
        if(reader.end_overload(vals))
          return (Value) std_numeric_min(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"clamp",
      ASTERIA_BINDING(
        "std.numeric.clamp", "value, lower, upper",
        Argument_Reader&& reader)
      {
        Value value, lower, upper;

        reader.start_overload();
        reader.optional(value);
        reader.optional(lower);
        reader.optional(upper);
        if(reader.end_overload())
          return (Value) std_numeric_clamp(value, lower, upper);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"round",
      ASTERIA_BINDING(
        "std.numeric.round", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_round(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_round(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"iround",
      ASTERIA_BINDING(
        "std.numeric.iround", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_iround(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_iround(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"floor",
      ASTERIA_BINDING(
        "std.numeric.floor", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_floor(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_floor(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"ifloor",
      ASTERIA_BINDING(
        "std.numeric.floor", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_ifloor(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_ifloor(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"ceil",
      ASTERIA_BINDING(
        "std.numeric.ceil", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_ceil(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_ceil(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"iceil",
      ASTERIA_BINDING(
        "std.numeric.iceil", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_iceil(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_iceil(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"trunc",
      ASTERIA_BINDING(
        "std.numeric.trunc", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_trunc(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_trunc(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"itrunc",
      ASTERIA_BINDING(
        "std.numeric.itrunc", "value",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;

        reader.start_overload();
        reader.required(ival);
        if(reader.end_overload())
          return (Value) std_numeric_itrunc(ival);

        reader.start_overload();
        reader.required(fval);
        if(reader.end_overload())
          return (Value) std_numeric_itrunc(fval);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"random",
      ASTERIA_BINDING(
        "std.numeric.random", "[limit]",
        Global_Context& global, Argument_Reader&& reader)
      {
        optV_real lim;

        reader.start_overload();
        reader.optional(lim);
        if(reader.end_overload())
          return (Value) std_numeric_random(global, lim);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"remainder",
      ASTERIA_BINDING(
        "std.numeric.remainder", "x, y",
        Argument_Reader&& reader)
      {
        V_real x, y;

        reader.start_overload();
        reader.required(x);
        reader.required(y);
        if(reader.end_overload())
          return (Value) std_numeric_remainder(x, y);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"frexp",
      ASTERIA_BINDING(
        "std.numeric.frexp", "x",
        Argument_Reader&& reader)
      {
        V_real val;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_frexp(val);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"ldexp",
      ASTERIA_BINDING(
        "std.numeric.ldexp", "frac, exp",
        Argument_Reader&& reader)
      {
        V_real frac;
        V_integer exp;

        reader.start_overload();
        reader.required(frac);
        reader.required(exp);
        if(reader.end_overload())
          return (Value) std_numeric_ldexp(frac, exp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"rotl",
      ASTERIA_BINDING(
        "std.numeric.rotl", "m, x, n",
        Argument_Reader&& reader)
      {
        V_integer m;
        V_integer x;
        V_integer sh;

        reader.start_overload();
        reader.required(m);
        reader.required(x);
        reader.required(sh);
        if(reader.end_overload())
          return (Value) std_numeric_rotl(m, x, sh);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"rotr",
      ASTERIA_BINDING(
        "std.numeric.rotr", "m, x, n",
        Argument_Reader&& reader)
      {
        V_integer m;
        V_integer x;
        V_integer sh;

        reader.start_overload();
        reader.required(m);
        reader.required(x);
        reader.required(sh);
        if(reader.end_overload())
          return (Value) std_numeric_rotr(m, x, sh);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"format",
      ASTERIA_BINDING(
        "std.numeric.format", "value, [base, [ebase]]",
        Argument_Reader&& reader)
      {
        V_integer ival;
        V_real fval;
        optV_integer base;
        optV_integer ebase;

        reader.start_overload();
        reader.required(ival);
        reader.optional(base);
        reader.optional(ebase);
        if(reader.end_overload())
          return (Value) std_numeric_format(ival, base, ebase);

        reader.start_overload();
        reader.required(fval);
        reader.optional(base);
        reader.optional(ebase);
        if(reader.end_overload())
          return (Value) std_numeric_format(fval, base, ebase);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"parse",
      ASTERIA_BINDING(
        "std.numeric.parse", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_parse(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_i8",
      ASTERIA_BINDING(
        "std.numeric.pack_i8", "values",
        Argument_Reader&& reader)
      {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i8(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i8(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_i8",
      ASTERIA_BINDING(
        "std.numeric.unpack_i8", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_i8(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_i16be",
      ASTERIA_BINDING(
        "std.numeric.pack_i16be", "values",
        Argument_Reader&& reader)
      {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i16be(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i16be(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_i16be",
      ASTERIA_BINDING(
        "std.numeric.unpack_i16be", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_i16be(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_i16le",
      ASTERIA_BINDING(
        "std.numeric.pack_i16le", "values",
        Argument_Reader&& reader)
      {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i16le(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i16le(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_i16le",
      ASTERIA_BINDING(
        "std.numeric.unpack_i16le", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_i16le(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_i32be",
      ASTERIA_BINDING(
        "std.numeric.pack_i32be", "values",
        Argument_Reader&& reader)
      {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i32be(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i32be(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_i32be",
      ASTERIA_BINDING(
        "std.numeric.unpack_i32be", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_i32be(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_i32le",
      ASTERIA_BINDING(
        "std.numeric.pack_i32le", "values",
        Argument_Reader&& reader)
      {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i32le(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i32le(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_i32le",
      ASTERIA_BINDING(
        "std.numeric.unpack_i32le", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_i32le(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_i64be",
      ASTERIA_BINDING(
        "std.numeric.pack_i64be", "values",
        Argument_Reader&& reader)
      {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i64be(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i64be(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_i64be",
      ASTERIA_BINDING(
        "std.numeric.unpack_i64be", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_i64be(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_i64le",
      ASTERIA_BINDING(
        "std.numeric.pack_i64le", "values",
        Argument_Reader&& reader)
      {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i64le(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_i64le(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_i64le",
      ASTERIA_BINDING(
        "std.numeric.unpack_i64le", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_i64le(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_f32be",
      ASTERIA_BINDING(
        "std.numeric.pack_f32be", "values",
        Argument_Reader&& reader)
      {
        V_real val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_f32be(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_f32be(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_f32be",
      ASTERIA_BINDING(
        "std.numeric.unpack_f32be", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_f32be(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_f32le",
      ASTERIA_BINDING(
        "std.numeric.pack_f32le", "values",
        Argument_Reader&& reader)
      {
        V_real val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_f32le(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_f32le(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_f32le",
      ASTERIA_BINDING(
        "std.numeric.unpack_f32le", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_f32le(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_f64be",
      ASTERIA_BINDING(
        "std.numeric.pack_f64be", "values",
        Argument_Reader&& reader)
      {
        V_real val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_f64be(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_f64be(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_f64be",
      ASTERIA_BINDING(
        "std.numeric.unpack_f64be", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_f64be(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pack_f64le",
      ASTERIA_BINDING(
        "std.numeric.pack_f64le", "values",
        Argument_Reader&& reader)
      {
        V_real val;
        V_array vals;

        reader.start_overload();
        reader.required(val);
        if(reader.end_overload())
          return (Value) std_numeric_pack_f64le(val);

        reader.start_overload();
        reader.required(vals);
        if(reader.end_overload())
          return (Value) std_numeric_pack_f64le(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"unpack_f64le",
      ASTERIA_BINDING(
        "std.numeric.unpack_f64le", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_numeric_unpack_f64le(text);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
