// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "numeric.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/random_engine.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

int64_t
do_verify_bounds(int64_t lower, int64_t upper)
  {
    if(!(lower <= upper))
      ASTERIA_THROW_RUNTIME_ERROR(
          "bounds not valid (`$1` is not less than or equal to `$2`)",
          lower, upper);

    return upper;
  }

double
do_verify_bounds(double lower, double upper)
  {
    if(!::std::islessequal(lower, upper))
      ASTERIA_THROW_RUNTIME_ERROR(
          "bounds not valid (`$1` is not less than or equal to `$2`)",
          lower, upper);

    return upper;
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

    // Format the integer.
    // If the exponent is negative, a minus sign will have been added.
    nump.put_DI(exp, 2);

    // Append significant figures.
    text.append(nump.begin(), nump.end());
    return text;
  }

inline void
bswap_be(int8_t& value) noexcept
  {
    (void)value;
  }

inline void
bswap_le(int8_t& value) noexcept
  {
    (void)value;
  }

inline void
bswap_be(int16_t& value) noexcept
  {
    uint16_t buf;
    ::std::memcpy(&buf, &value, sizeof(value));
    buf = be16toh(buf);
    ::std::memcpy(&value, &buf, sizeof(value));
  }

inline void
bswap_le(int16_t& value) noexcept
  {
    uint16_t buf;
    ::std::memcpy(&buf, &value, sizeof(value));
    buf = le16toh(buf);
    ::std::memcpy(&value, &buf, sizeof(value));
  }

inline void
bswap_be(int32_t& value) noexcept
  {
    uint32_t buf;
    ::std::memcpy(&buf, &value, sizeof(value));
    buf = be32toh(buf);
    ::std::memcpy(&value, &buf, sizeof(value));
  }

inline void
bswap_le(int32_t& value) noexcept
  {
    uint32_t buf;
    ::std::memcpy(&buf, &value, sizeof(value));
    buf = le32toh(buf);
    ::std::memcpy(&value, &buf, sizeof(value));
  }

inline void
bswap_be(int64_t& value) noexcept
  {
    uint64_t buf;
    ::std::memcpy(&buf, &value, sizeof(value));
    buf = be64toh(buf);
    ::std::memcpy(&value, &buf, sizeof(value));
  }

inline void
bswap_le(int64_t& value) noexcept
  {
    uint64_t buf;
    ::std::memcpy(&buf, &value, sizeof(value));
    buf = le64toh(buf);
    ::std::memcpy(&value, &buf, sizeof(value));
  }

inline void
bswap_be(float& value) noexcept
  {
    uint32_t buf;
    ::std::memcpy(&buf, &value, sizeof(value));
    buf = be32toh(buf);
    ::std::memcpy(&value, &buf, sizeof(value));
  }

inline void
bswap_le(float& value) noexcept
  {
    uint32_t buf;
    ::std::memcpy(&buf, &value, sizeof(value));
    buf = le32toh(buf);
    ::std::memcpy(&value, &buf, sizeof(value));
  }

inline void
bswap_be(double& value) noexcept
  {
    uint64_t buf;
    ::std::memcpy(&buf, &value, sizeof(value));
    buf = be64toh(buf);
    ::std::memcpy(&value, &buf, sizeof(value));
  }

inline void
bswap_le(double& value) noexcept
  {
    uint64_t buf;
    ::std::memcpy(&buf, &value, sizeof(value));
    buf = le64toh(buf);
    ::std::memcpy(&value, &buf, sizeof(value));
  }

template<typename WordT, typename ValueT>
V_string
do_pack(void bswap(WordT&), const ValueT& value)
  {
    union { WordT word;  char bytes[1];  } un;
    V_string text;

    un.word = static_cast<WordT>(value);
    (*bswap)(un.word);
    text.append(un.bytes, sizeof(un));
    return text;
  }

template<typename WordT, typename ValueT>
V_string
do_pack(void bswap(WordT&), ValueT (Value::*access)() const, const V_array& values)
  {
    union { WordT word;  char bytes[1];  } un;
    V_string text;

    text.reserve(values.size() * sizeof(un));
    for(const auto& val : values) {
      un.word = static_cast<WordT>((val.*access)());
      (*bswap)(un.word);
      text.append(un.bytes, sizeof(un));
    }
    return text;
  }

template<typename WordT, typename ValueT>
V_array
do_unpack(void bswap(WordT&), const V_string& text)
  {
    union { WordT word;  char bytes[1];  } un;
    V_array values;

    if(text.size() / sizeof(un) * sizeof(un) != text.size())
      ASTERIA_THROW_RUNTIME_ERROR(
            "string length `$1` not divisible by `$2`",
            text.size(), sizeof(un));

    values.reserve(text.size() / sizeof(un));
    size_t offset = 0;
    while(text.copy(offset, un.bytes, sizeof(un)) != 0) {
      offset += sizeof(un);
      (*bswap)(un.word);
      values.emplace_back(static_cast<ValueT>(un.word));
    }
    return values;
  }

}  // namespace

V_integer
std_numeric_abs(V_integer value)
  {
    if(value == INT64_MIN)
      ASTERIA_THROW_RUNTIME_ERROR(
          "integer absolute value overflow (value `$1`)",
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
          ASTERIA_THROW_RUNTIME_ERROR(
              "values not comparable (operands were `$1` and `$2`)",
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
          ASTERIA_THROW_RUNTIME_ERROR(
              "values not comparable (operands were `$1` and `$2`)", cmp, val);

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
          ASTERIA_THROW_RUNTIME_ERROR(
              "random number limit shall not be zero");

        case FP_INFINITE:
        case FP_NAN:
          ASTERIA_THROW_RUNTIME_ERROR(
              "random number limit shall be finite (limit `$1`)", *limit);

        default:
          ratio *= *limit;
      }
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
      ASTERIA_THROW_RUNTIME_ERROR(
          "invalid modulo bit count (`$1` is not between 0 and 64)", m);

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
      ASTERIA_THROW_RUNTIME_ERROR(
          "invalid modulo bit count (`$1` is not between 0 and 64)", m);

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
std_numeric_format(V_integer value, optV_integer base, optV_integer ebase)
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

        ASTERIA_THROW_RUNTIME_ERROR(
            "invalid exponent base for binary notation (`$1` is not 2)", *ebase);
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

        ASTERIA_THROW_RUNTIME_ERROR(
            "invalid exponent base for hexadecimal notation (`$1` is not 2)", *ebase);
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

        ASTERIA_THROW_RUNTIME_ERROR(
            "invalid exponent base for decimal notation (`$1` is not 10)", *ebase);
      }

      default:
        ASTERIA_THROW_RUNTIME_ERROR(
            "invalid number base (base `$1` is not one of { 2, 10, 16 })", *base);
    }
    return text;
  }

V_string
std_numeric_format(V_real value, optV_integer base, optV_integer ebase)
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
        ASTERIA_THROW_RUNTIME_ERROR(
            "invalid exponent base for binary notation (`$1` is not 2)", *ebase);
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

        ASTERIA_THROW_RUNTIME_ERROR(
            "invalid exponent base for hexadecimal notation (`$1` is not 2)", *ebase);
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

        ASTERIA_THROW_RUNTIME_ERROR(
            "invalid exponent base for decimal notation (`$1` is not 10)", *ebase);
      }

      default:
        ASTERIA_THROW_RUNTIME_ERROR(
            "invalid number base (base `$1` is not one of { 2, 10, 16 })", *base);
    }
    return text;
  }

Value
std_numeric_parse(V_string text)
  {
    static constexpr char s_spaces[] = " \f\n\r\t\v";
    auto tpos = text.find_first_not_of(s_spaces);
    if(tpos == V_string::npos)
      ASTERIA_THROW_RUNTIME_ERROR("blank string");

    auto fbptr = text.data() + tpos;
    auto ibptr = fbptr;
    auto eptr = text.data() + text.find_last_not_of(s_spaces) + 1;

    // Try parsing the string as a real number first.
    ::rocket::ascii_numget numg;
    if(!numg.parse_F(fbptr, eptr))
      ASTERIA_THROW_RUNTIME_ERROR(
          "string not convertible to a number (text `$1`)", text);

    if(fbptr != eptr)
      ASTERIA_THROW_RUNTIME_ERROR(
          "non-numeric character in string (character `$1`)", *fbptr);

    V_real fval;
    numg.cast_F(fval, -HUGE_VAL, HUGE_VAL);

    // Check whether the value fits in an integer.
    V_integer ival;
    if(numg.parse_I(ibptr, eptr)  // accepted?
       && (ibptr == eptr)         // no junk character?
       && numg.cast_I(ival, INT64_MIN, INT64_MAX))  // within range?
      return ival;

    // Accept any underflowed or overflowed result as a real.
    return fval;
  }

V_string
std_numeric_pack_i8(V_integer value)
  {
    return do_pack<int8_t, V_integer>(bswap_be, value);
  }

V_string
std_numeric_pack_i8(V_array values)
  {
    return do_pack<int8_t, V_integer>(bswap_be, &Value::as_integer, values);
  }

V_array
std_numeric_unpack_i8(V_string text)
  {
    return do_unpack<int8_t, V_integer>(bswap_be, text);
  }

V_string
std_numeric_pack_i16be(V_integer value)
  {
    return do_pack<int16_t, V_integer>(bswap_be, value);
  }

V_string
std_numeric_pack_i16be(V_array values)
  {
    return do_pack<int16_t, V_integer>(bswap_be, &Value::as_integer, values);
  }

V_array
std_numeric_unpack_i16be(V_string text)
  {
    return do_unpack<int16_t, V_integer>(bswap_be, text);
  }

V_string
std_numeric_pack_i16le(V_integer value)
  {
    return do_pack<int16_t, V_integer>(bswap_le, value);
  }

V_string
std_numeric_pack_i16le(V_array values)
  {
    return do_pack<int16_t, V_integer>(bswap_le, &Value::as_integer, values);
  }

V_array
std_numeric_unpack_i16le(V_string text)
  {
    return do_unpack<int16_t, V_integer>(bswap_le, text);
  }

V_string
std_numeric_pack_i32be(V_integer value)
  {
    return do_pack<int32_t, V_integer>(bswap_be, value);
  }

V_string
std_numeric_pack_i32be(V_array values)
  {
    return do_pack<int32_t, V_integer>(bswap_be, &Value::as_integer, values);
  }

V_array
std_numeric_unpack_i32be(V_string text)
  {
    return do_unpack<int32_t, V_integer>(bswap_be, text);
  }

V_string
std_numeric_pack_i32le(V_integer value)
  {
    return do_pack<int32_t, V_integer>(bswap_le, value);
  }

V_string
std_numeric_pack_i32le(V_array values)
  {
    return do_pack<int32_t, V_integer>(bswap_le, &Value::as_integer, values);
  }

V_array
std_numeric_unpack_i32le(V_string text)
  {
    return do_unpack<int32_t, V_integer>(bswap_le, text);
  }

V_string
std_numeric_pack_i64be(V_integer value)
  {
    return do_pack<int64_t, V_integer>(bswap_be, value);
  }

V_string
std_numeric_pack_i64be(V_array values)
  {
    return do_pack<int64_t, V_integer>(bswap_be, &Value::as_integer, values);
  }

V_array
std_numeric_unpack_i64be(V_string text)
  {
    return do_unpack<int64_t, V_integer>(bswap_be, text);
  }

V_string
std_numeric_pack_i64le(V_integer value)
  {
    return do_pack<int64_t, V_integer>(bswap_le, value);
  }

V_string
std_numeric_pack_i64le(V_array values)
  {
    return do_pack<int64_t, V_integer>(bswap_le, &Value::as_integer, values);
  }

V_array
std_numeric_unpack_i64le(V_string text)
  {
    return do_unpack<int64_t, V_integer>(bswap_le, text);
  }

V_string
std_numeric_pack_f32be(V_real value)
  {
    return do_pack<float, V_real>(bswap_be, value);
  }

V_string
std_numeric_pack_f32be(V_array values)
  {
    return do_pack<float, V_real>(bswap_be, &Value::as_real, values);
  }

V_array
std_numeric_unpack_f32be(V_string text)
  {
    return do_unpack<float, V_real>(bswap_be, text);
  }

V_string
std_numeric_pack_f32le(V_real value)
  {
    return do_pack<float, V_real>(bswap_le, value);
  }

V_string
std_numeric_pack_f32le(V_array values)
  {
    return do_pack<float, V_real>(bswap_le, &Value::as_real, values);
  }

V_array
std_numeric_unpack_f32le(V_string text)
  {
    return do_unpack<float, V_real>(bswap_le, text);
  }

V_string
std_numeric_pack_f64be(V_real value)
  {
    return do_pack<double, V_real>(bswap_be, value);
  }

V_string
std_numeric_pack_f64be(V_array values)
  {
    return do_pack<double, V_real>(bswap_be, &Value::as_real, values);
  }

V_array
std_numeric_unpack_f64be(V_string text)
  {
    return do_unpack<double, V_real>(bswap_be, text);
  }

V_string
std_numeric_pack_f64le(V_real value)
  {
    return do_pack<double, V_real>(bswap_le, value);
  }

V_string
std_numeric_pack_f64le(V_array values)
  {
    return do_pack<double, V_real>(bswap_le, &Value::as_real, values);
  }

V_array
std_numeric_unpack_f64le(V_string text)
  {
    return do_unpack<double, V_real>(bswap_le, text);
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

    result.insert_or_assign(sref("sign"),
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

    result.insert_or_assign(sref("is_finite"),
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

    result.insert_or_assign(sref("is_infinity"),
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

    result.insert_or_assign(sref("is_nan"),
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

    result.insert_or_assign(sref("max"),
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

    result.insert_or_assign(sref("min"),
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

    result.insert_or_assign(sref("clamp"),
      ASTERIA_BINDING(
        "std.numeric.clamp", "value, lower, upper",
        Argument_Reader&& reader)
      {
        V_integer ival, ilo, iup;
        V_real fval, flo, fup;

        reader.start_overload();
        reader.required(ival);
        reader.required(ilo);
        reader.required(iup);
        if(reader.end_overload())
          return (Value) std_numeric_clamp(ival, ilo, iup);

        reader.start_overload();
        reader.required(fval);
        reader.required(flo);
        reader.required(fup);
        if(reader.end_overload())
          return (Value) std_numeric_clamp(fval, flo, fup);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("round"),
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

    result.insert_or_assign(sref("iround"),
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

    result.insert_or_assign(sref("floor"),
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

    result.insert_or_assign(sref("ifloor"),
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

    result.insert_or_assign(sref("ceil"),
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

    result.insert_or_assign(sref("iceil"),
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

    result.insert_or_assign(sref("trunc"),
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

    result.insert_or_assign(sref("itrunc"),
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

    result.insert_or_assign(sref("random"),
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

    result.insert_or_assign(sref("remainder"),
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

    result.insert_or_assign(sref("frexp"),
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

    result.insert_or_assign(sref("ldexp"),
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

    result.insert_or_assign(sref("rotl"),
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

    result.insert_or_assign(sref("rotr"),
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

    result.insert_or_assign(sref("format"),
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

    result.insert_or_assign(sref("parse"),
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

    result.insert_or_assign(sref("pack_i8"),
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

    result.insert_or_assign(sref("unpack_i8"),
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

    result.insert_or_assign(sref("pack_i16be"),
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

    result.insert_or_assign(sref("unpack_i16be"),
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

    result.insert_or_assign(sref("pack_i16le"),
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

    result.insert_or_assign(sref("unpack_i16le"),
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

    result.insert_or_assign(sref("pack_i32be"),
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

    result.insert_or_assign(sref("unpack_i32be"),
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

    result.insert_or_assign(sref("pack_i32le"),
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

    result.insert_or_assign(sref("unpack_i32le"),
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

    result.insert_or_assign(sref("pack_i64be"),
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

    result.insert_or_assign(sref("unpack_i64be"),
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

    result.insert_or_assign(sref("pack_i64le"),
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

    result.insert_or_assign(sref("unpack_i64le"),
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

    result.insert_or_assign(sref("pack_f32be"),
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

    result.insert_or_assign(sref("unpack_f32be"),
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

    result.insert_or_assign(sref("pack_f32le"),
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

    result.insert_or_assign(sref("unpack_f32le"),
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

    result.insert_or_assign(sref("pack_f64be"),
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

    result.insert_or_assign(sref("unpack_f64be"),
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

    result.insert_or_assign(sref("pack_f64le"),
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

    result.insert_or_assign(sref("unpack_f64le"),
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
