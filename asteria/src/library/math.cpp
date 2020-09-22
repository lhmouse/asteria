// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "math.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../utilities.hpp"

namespace asteria {
namespace {

constexpr double s_const_e    = 2.7182818284590452353602874713526624977572470937000;
constexpr double s_const_pi   = 3.1415926535897932384626433832795028841971693993751;
constexpr double s_const_lb10 = 3.3219280948873623478703194294893901758648313930246;

}  // namespace

V_real
std_math_exp(V_real y, optV_real base)
  {
    if(!base)
      return ::exp(y);

    if(*base == 2)
      return ::exp2(y);

    if(*base == s_const_e)
      return ::exp(y);

    return ::pow(*base, y);
  }

V_real
std_math_expm1(V_real y)
  {
    return ::expm1(y);
  }

V_real
std_math_pow(V_real x, V_real y)
  {
    return ::pow(x, y);
  }

V_real
std_math_log(V_real x, optV_real base)
  {
    if(!base)
      return ::log(x);

    if(*base == 2)
      return ::log2(x);

    if(*base == 10)
      return ::log10(x);

    if((*base == 1) || (*base <= 0))
      return ::std::numeric_limits<double>::quiet_NaN();

    return ::log2(x) / ::log2(*base);
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
    double sinv, cosv;
    ::sincos(x, &sinv, &cosv);
    return ::std::make_pair(sinv, cosv);
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
    // Return zero if no argument is provided.
    if(values.size() == 0)
      return 0;

    // Return the absolute value of the only argument.
    if(values.size() == 1)
      return ::fabs(values[0].convert_to_real());

    // Call the C `hypot()` function for every two values.
    auto res = ::hypot(values[0].convert_to_real(), values[1].convert_to_real());
    for(size_t i = 2;  i < values.size();  ++i)
      res = ::hypot(res, values[i].convert_to_real());
    return res;
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
    //===================================================================
    // `std.math.e`
    //===================================================================
    result.insert_or_assign(::rocket::sref("e"),
      V_real(
        // The base of the natural logarithm.
        s_const_e
      ));

    //===================================================================
    // `std.math.pi`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pi"),
      V_real(
        // The ratio of a circle's circumference to its diameter.
        s_const_pi
      ));

    //===================================================================
    // `std.math.lb10`
    //===================================================================
    result.insert_or_assign(::rocket::sref("real_max"),
      V_real(
        // The binary logarithm of the integer ten.
        s_const_lb10
      ));

    //===================================================================
    // `std.math.exp()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("exp"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.exp(y, [base])`

  * Calculates `base` raised to the power `y`. If `base` is absent,
    `e` is assumed. This function is equivalent to `pow(base, y)`.

  * Returns the power as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.exp"), ::rocket::cref(args));
    // Parse arguments.
    V_real y;
    optV_real base;
    if(reader.I().v(y).o(base).F()) {
      Reference_root::S_temporary xref = { std_math_exp(::std::move(y), ::std::move(base)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.expm1()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("expm1"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.expm1(y)`

  * Calculates `exp(y) - 1` without losing precision when `y` is
    close to zero.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.expm1"), ::rocket::cref(args));
    // Parse arguments.
    V_real y;
    if(reader.I().v(y).F()) {
      Reference_root::S_temporary xref = { std_math_expm1(::std::move(y)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.pow()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pow"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.pow(x, y)`

  * Calculates `x` raised to the power `y`. According to C99, when
    `x` is `1` or `y` is `0`, the result is always `1`, even when
    the other argument is an infinity or NaN.

  * Returns the power as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.pow"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    V_real y;
    if(reader.I().v(x).v(y).F()) {
      Reference_root::S_temporary xref = { std_math_pow(::std::move(x), ::std::move(y)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.log()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("log"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.log(x, [base])`

  * Calculates the logarithm of `x` to `base`. If `base` is absent,
    `e` is assumed.

  * Returns the logarithm as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.log"), ::rocket::cref(args));
    // Parse arguments.
    V_real y;
    optV_real base;
    if(reader.I().v(y).o(base).F()) {
      Reference_root::S_temporary xref = { std_math_log(::std::move(y), ::std::move(base)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.log1p()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("log1p"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.log1p(x)`

  * Calculates `log(1 + x)` without losing precision when `x` is
    close to zero.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.log1p"), ::rocket::cref(args));
    // Parse arguments.
    V_real y;
    if(reader.I().v(y).F()) {
      Reference_root::S_temporary xref = { std_math_log1p(::std::move(y)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.sin()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sin"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.sin(x)`

  * Calculates the sine of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.sin"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_sin(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.cos()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("cos"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.cos(x)`

  * Calculates the cosine of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.cos"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_cos(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.sincos()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sincos"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.sincos(x)`

 * Calculates the sine and cosine of `x` in radians.

 * Returns an array of two reals. The first element is the sine
   and the other is the cosine.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.sincos"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_sincos(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.tan()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("tan"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.tan(x)`

  * Calculates the tangent of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.tan"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_tan(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.asin()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("asin"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.asin(x)`

  * Calculates the inverse sine of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.asin"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_asin(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.acos()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("acos"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.acos(x)`

  * Calculates the inverse cosine of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.acos"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_acos(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.atan()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("atan"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.atan(x)`

  * Calculates the inverse tangent of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.atan"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_atan(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.atan2()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("atan2"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.atan2(y, x)`

  * Calculates the angle of the vector `<x,y>` in radians.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.atan2"), ::rocket::cref(args));
    // Parse arguments.
    V_real y;
    V_real x;
    if(reader.I().v(y).v(x).F()) {
      Reference_root::S_temporary xref = { std_math_atan2(::std::move(y), ::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.hypot()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("hypot"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.hypot(...)`

  * Calculates the length of the n-dimension vector defined by all
    arguments. If no argument is provided, this function returns
    zero. If any argument is an infinity, this function returns a
    positive infinity; otherwise, if any argument is a NaN, this
    function returns a NaN.

  * Returns the length as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.hypot"), ::rocket::cref(args));
    // Parse variadic arguments.
    cow_vector<Value> values;
    if(reader.I().F(values)) {
      Reference_root::S_temporary xref = { std_math_hypot(::std::move(values)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.sinh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sinh"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.sinh(x)`

  * Calculates the hyperbolic sine of `x`.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.sinh"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_sinh(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.cosh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("cosh"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.cosh(x)`

  * Calculates the hyperbolic cosine of `x`.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.cosh"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_cosh(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.tanh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("tanh"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.tanh(x)`

  * Calculates the hyperbolic tangent of `x`.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.tanh"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_tanh(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.asinh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("asinh"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.asinh(x)`

  * Calculates the inverse hyperbolic sine of `x`.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.asinh"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_asinh(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.acosh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("acosh"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.acosh(x)`

  * Calculates the inverse hyperbolic cosine of `x`.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.acosh"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_acosh(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.atanh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("atanh"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.atanh(x)`

  * Calculates the inverse hyperbolic tangent of `x`.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.atanh"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_atanh(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.erf()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("erf"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.erf(x)`

  * Calculates the error function of `x`.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.erf"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_erf(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.cerf()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("cerf"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.cerf(x)`

  * Calculates the complementary error function of `x`.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.cerf"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_cerf(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.gamma()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gamma"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.gamma(x)`

  * Calculates the Gamma function of `x`.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.gamma"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_gamma(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.math.lgamma()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("lgamma"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.lgamma(x)`

  * Calculates the natural logarithm of the absolute value of the
    Gamma function of `x`.

  * Returns the result as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.math.lgamma"), ::rocket::cref(args));
    // Parse arguments.
    V_real x;
    if(reader.I().v(x).F()) {
      Reference_root::S_temporary xref = { std_math_lgamma(::std::move(x)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));
  }

}  // namespace asteria
