// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "math.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/global_context.hpp"
#include "../utils.hpp"
namespace asteria {

V_real
std_math_exp(V_real y)
  {
    return ::exp(y);
  }

V_real
std_math_exp(V_real base, V_real y)
  {
    if(base == 2.7182818284590452353602874713527)
      return ::exp(y);

    if(base == 2)
      return ::exp2(y);

    if(base == 10)
      return ::exp10(y);

    return ::pow(base, y);
  }

V_real
std_math_log(V_real x)
  {
    return ::log(x);
  }

V_real
std_math_log(V_real base, V_real x)
  {
    if(base == 2.7182818284590452353602874713527)
      return ::log(x);

    if(base == 2)
      return ::log2(x);

    if(base == 10)
      return ::log10(x);

    auto r = ::log2(base);
    return ::std::isgreater(r, 1) ? ::log2(x) / r
             : ::std::numeric_limits<double>::quiet_NaN();
  }

V_real
std_math_expm1(V_real y)
  {
    return ::expm1(y);
  }

V_real
std_math_log1p(V_real x)
  {
    return ::log1p(x);
  }

V_real
std_math_sin(V_real x)
  {
    return ::sin(x);
  }

V_real
std_math_cos(V_real x)
  {
    return ::cos(x);
  }

pair<V_real, V_real>
std_math_sincos(V_real x)
  {
    double sin, cos;
    ::sincos(x, &sin, &cos);
    return ::std::make_pair(sin, cos);
  }

V_real
std_math_tan(V_real x)
  {
    return ::tan(x);
  }

V_real
std_math_asin(V_real x)
  {
    return ::asin(x);
  }

V_real
std_math_acos(V_real x)
  {
    return ::acos(x);
  }

V_real
std_math_atan(V_real x)
  {
    return ::atan(x);
  }

V_real
std_math_atan2(V_real y, V_real x)
  {
    return ::atan2(y, x);
  }

V_real
std_math_hypot(cow_vector<Value> values)
  {
    V_real len = 0;
    for(const auto& val : values)
      if(!val.is_null())
        len = ::hypot(len, val.as_real());
    return len;
  }

V_real
std_math_sinh(V_real x)
  {
    return ::sinh(x);
  }

V_real
std_math_cosh(V_real x)
  {
    return ::cosh(x);
  }

V_real
std_math_tanh(V_real x)
  {
    return ::tanh(x);
  }

V_real
std_math_asinh(V_real x)
  {
    return ::asinh(x);
  }

V_real
std_math_acosh(V_real x)
  {
    return ::acosh(x);
  }

V_real
std_math_atanh(V_real x)
  {
    return ::atanh(x);
  }

V_real
std_math_erf(V_real x)
  {
    return ::erf(x);
  }

V_real
std_math_cerf(V_real x)
  {
    return ::erfc(x);
  }

V_real
std_math_gamma(V_real x)
  {
    return ::tgamma(x);
  }

V_real
std_math_lgamma(V_real x)
  {
    return ::lgamma(x);
  }

void
create_bindings_math(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("pi"),
      V_real(
        3.1415926535897932384626433832795
      ));

    result.insert_or_assign(sref("rad"),
      V_real(
        57.295779513082320876798154814105
      ));

    result.insert_or_assign(sref("deg"),
      V_real(
        0.01745329251994329576923690768489
      ));

    result.insert_or_assign(sref("e"),
      V_real(
        2.7182818284590452353602874713527
      ));

    result.insert_or_assign(sref("sqrt2"),
      V_real(
        1.4142135623730950488016887242097
      ));

    result.insert_or_assign(sref("sqrt3"),
      V_real(
        1.7320508075688772935274463415059
      ));

    result.insert_or_assign(sref("cbrt2"),
      V_real(
        1.2599210498948731647672106072782
      ));

    result.insert_or_assign(sref("lg2"),
      V_real(
        0.30102999566398119521373889472449
      ));

    result.insert_or_assign(sref("lb10"),
      V_real(
        3.3219280948873623478703194294894
      ));

    result.insert_or_assign(sref("exp"),
      ASTERIA_BINDING(
        "std.math.exp", "[base], y",
        Argument_Reader&& reader)
      {
        V_real base, y;

        reader.start_overload();
        reader.required(y);
        if(reader.end_overload())
          return (Value) std_math_exp(y);

        reader.start_overload();
        reader.required(base);
        reader.required(y);
        if(reader.end_overload())
          return (Value) std_math_exp(base, y);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("log"),
      ASTERIA_BINDING(
        "std.math.log", "[base], x",
        Argument_Reader&& reader)
      {
        V_real base, x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_log(x);

        reader.start_overload();
        reader.required(base);
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_log(base, x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("expm1"),
      ASTERIA_BINDING(
        "std.math.expm1", "",
        Argument_Reader&& reader)
      {
        V_real y;

        reader.start_overload();
        reader.required(y);
        if(reader.end_overload())
          return (Value) std_math_expm1(y);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("log1p"),
      ASTERIA_BINDING(
        "std.math.log1p", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_log1p(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sin"),
      ASTERIA_BINDING(
        "std.math.sin", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_sin(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("cos"),
      ASTERIA_BINDING(
        "std.math.cos", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_cos(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sincos"),
      ASTERIA_BINDING(
        "std.math.sincos", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_sincos(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("tan"),
      ASTERIA_BINDING(
        "std.math.tan", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_tan(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("asin"),
      ASTERIA_BINDING(
        "std.math.asin", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_asin(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("acos"),
      ASTERIA_BINDING(
        "std.math.acos", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_acos(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("atan"),
      ASTERIA_BINDING(
        "std.math.atan", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_atan(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("atan2"),
      ASTERIA_BINDING(
        "std.math.atan2", "y, x",
        Argument_Reader&& reader)
      {
        V_real y;
        V_real x;

        reader.start_overload();
        reader.required(y);
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_atan2(y, x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("hypot"),
      ASTERIA_BINDING(
        "std.math.hypot", "...",
        Argument_Reader&& reader)
      {
        cow_vector<Value> vals;

        reader.start_overload();
        if(reader.end_overload(vals))
          return (Value) std_math_hypot(vals);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sinh"),
      ASTERIA_BINDING(
        "std.math.sinh", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_sinh(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("cosh"),
      ASTERIA_BINDING(
        "std.math.cosh", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_cosh(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("tanh"),
      ASTERIA_BINDING(
        "std.math.tanh", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_tanh(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("asinh"),
      ASTERIA_BINDING(
        "std.math.asinh", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_asinh(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("acosh"),
      ASTERIA_BINDING(
        "std.math.acosh", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_acosh(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("atanh"),
      ASTERIA_BINDING(
        "std.math.atanh", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_atanh(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("erf"),
      ASTERIA_BINDING(
        "std.math.erf", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_erf(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("cerf"),
      ASTERIA_BINDING(
        "std.math.cerf", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_cerf(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("gamma"),
      ASTERIA_BINDING(
        "std.math.gamma", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_gamma(x);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("lgamma"),
      ASTERIA_BINDING(
        "std.math.lgamma", "x",
        Argument_Reader&& reader)
      {
        V_real x;

        reader.start_overload();
        reader.required(x);
        if(reader.end_overload())
          return (Value) std_math_lgamma(x);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
