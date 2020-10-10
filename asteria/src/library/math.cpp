// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "math.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../util.hpp"

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
        len = ::hypot(len, val.convert_to_real());
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
    result.insert_or_assign(::rocket::sref("pi"),
      V_real(
        3.1415926535897932384626433832795
      ));

    result.insert_or_assign(::rocket::sref("rad"),
      V_real(
        57.295779513082320876798154814105
      ));

    result.insert_or_assign(::rocket::sref("deg"),
      V_real(
        0.01745329251994329576923690768489
      ));

    result.insert_or_assign(::rocket::sref("e"),
      V_real(
        2.7182818284590452353602874713527
      ));

    result.insert_or_assign(::rocket::sref("sqrt2"),
      V_real(
        1.4142135623730950488016887242097
      ));

    result.insert_or_assign(::rocket::sref("sqrt3"),
      V_real(
        1.7320508075688772935274463415059
      ));

    result.insert_or_assign(::rocket::sref("cbrt2"),
      V_real(
        1.2599210498948731647672106072782
      ));

    result.insert_or_assign(::rocket::sref("lg2"),
      V_real(
        0.30102999566398119521373889472449
      ));

    result.insert_or_assign(::rocket::sref("lb10"),
      V_real(
        3.3219280948873623478703194294894
      ));

    result.insert_or_assign(::rocket::sref("exp"),
      ASTERIA_BINDING_BEGIN("std.math.exp", self, global, reader) {
        V_real base;
        V_real y;

        reader.start_overload();
        reader.required(y);      // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_exp, y);

        reader.start_overload();
        reader.required(base);   // base
        reader.required(y);      // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_exp, base, y);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("log"),
      ASTERIA_BINDING_BEGIN("std.math.log", self, global, reader) {
        V_real base;
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_log, x);

        reader.start_overload();
        reader.required(base);   // base
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_log, base, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("expm1"),
      ASTERIA_BINDING_BEGIN("std.math.expm1", self, global, reader) {
        V_real y;

        reader.start_overload();
        reader.required(y);      // y
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_expm1, y);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("log1p"),
      ASTERIA_BINDING_BEGIN("std.math.log1p", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_log1p, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("sin"),
      ASTERIA_BINDING_BEGIN("std.math.sin", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_sin, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("cos"),
      ASTERIA_BINDING_BEGIN("std.math.cos", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_cos, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("sincos"),
      ASTERIA_BINDING_BEGIN("std.math.sincos", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_sincos, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("tan"),
      ASTERIA_BINDING_BEGIN("std.math.tan", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_tan, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("asin"),
      ASTERIA_BINDING_BEGIN("std.math.asin", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_asin, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("acos"),
      ASTERIA_BINDING_BEGIN("std.math.acos", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_acos, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("atan"),
      ASTERIA_BINDING_BEGIN("std.math.atan", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_atan, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("atan2"),
      ASTERIA_BINDING_BEGIN("std.math.atan2", self, global, reader) {
        V_real y;
        V_real x;

        reader.start_overload();
        reader.required(y);      // x
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_atan2, y, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("hypot"),
      ASTERIA_BINDING_BEGIN("std.math.hypot", self, global, reader) {
        cow_vector<Value> vals;

        reader.start_overload();
        if(reader.end_overload(vals))   // ...
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_hypot, vals);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("sinh"),
      ASTERIA_BINDING_BEGIN("std.math.sinh", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_sinh, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("cosh"),
      ASTERIA_BINDING_BEGIN("std.math.cosh", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_cosh, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("tanh"),
      ASTERIA_BINDING_BEGIN("std.math.tanh", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_tanh, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("asinh"),
      ASTERIA_BINDING_BEGIN("std.math.asinh", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_asinh, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("acosh"),
      ASTERIA_BINDING_BEGIN("std.math.acosh", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_acosh, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("atanh"),
      ASTERIA_BINDING_BEGIN("std.math.atanh", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_atanh, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("erf"),
      ASTERIA_BINDING_BEGIN("std.math.erf", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_erf, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("cerf"),
      ASTERIA_BINDING_BEGIN("std.math.cerf", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_cerf, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("gamma"),
      ASTERIA_BINDING_BEGIN("std.math.gamma", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_gamma, x);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("lgamma"),
      ASTERIA_BINDING_BEGIN("std.math.lgamma", self, global, reader) {
        V_real x;

        reader.start_overload();
        reader.required(x);      // x
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_math_lgamma, x);
      }
      ASTERIA_BINDING_END);
  }

}  // namespace asteria
