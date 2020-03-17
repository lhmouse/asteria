// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_math.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

constexpr Rval s_const_e    = 2.7182818284590452353602874713526624977572470937000;
constexpr Rval s_const_pi   = 3.1415926535897932384626433832795028841971693993751;
constexpr Rval s_const_lb10 = 3.3219280948873623478703194294893901758648313930246;

}  // namespace

Rval std_math_exp(Rval y, Ropt base)
  {
    if(!base) {
      return ::std::exp(y);
    }
    if(*base == 2) {
      return ::std::exp2(y);
    }
    if(*base == s_const_e) {
      return ::std::exp(y);
    }
    return ::std::pow(*base, y);
  }

Rval std_math_expm1(Rval y)
  {
    return ::std::expm1(y);
  }

Rval std_math_pow(Rval x, Rval y)
  {
    return ::std::pow(x, y);
  }

Rval std_math_log(Rval x, Ropt base)
  {
    if(!base) {
      return ::std::log(x);
    }
    if(*base == 2) {
      return ::std::log2(x);
    }
    if(*base == 10) {
      return ::std::log10(x);
    }
    if((*base == 1) || (*base <= 0)) {
      return ::std::numeric_limits<Rval>::quiet_NaN();
    }
    return ::std::log2(x) / ::std::log2(*base);
  }

Rval std_math_log1p(Rval x)
  {
    return ::std::log1p(x);
  }

Rval std_math_sin(Rval x)
  {
    return ::std::sin(x);
  }

Rval std_math_cos(Rval x)
  {
    return ::std::cos(x);
  }

Rval std_math_tan(Rval x)
  {
    return ::std::tan(x);
  }

Rval std_math_asin(Rval x)
  {
    return ::std::asin(x);
  }

Rval std_math_acos(Rval x)
  {
    return ::std::acos(x);
  }

Rval std_math_atan(Rval x)
  {
    return ::std::atan(x);
  }

Rval std_math_atan2(Rval y, Rval x)
  {
    return ::std::atan2(y, x);
  }

Rval std_math_hypot(cow_vector<Value> values)
  {
    switch(values.size()) {
    case 0: {
        // Return zero if no argument is provided.
        return 0;
      }
    case 1: {
        // Return the absolute value of the only argument.
        return ::std::fabs(values[0].convert_to_real());
      }
    default: {
        // Call the C `hypot()` function.
        auto result = ::std::hypot(values[0].convert_to_real(), values[1].convert_to_real());
        // Sum up all the other values.
        for(size_t i = 2;  i < values.size();  ++i) {
          result = ::std::hypot(result, values[i].convert_to_real());
        }
        return result;
      }
    }
  }

Rval std_math_sinh(Rval x)
  {
    return ::std::sinh(x);
  }

Rval std_math_cosh(Rval x)
  {
    return ::std::cosh(x);
  }

Rval std_math_tanh(Rval x)
  {
    return ::std::tanh(x);
  }

Rval std_math_asinh(Rval x)
  {
    return ::std::asinh(x);
  }

Rval std_math_acosh(Rval x)
  {
    return ::std::acosh(x);
  }

Rval std_math_atanh(Rval x)
  {
    return ::std::atanh(x);
  }

Rval std_math_erf(Rval x)
  {
    return ::std::erf(x);
  }

Rval std_math_cerf(Rval x)
  {
    return ::std::erfc(x);
  }

Rval std_math_gamma(Rval x)
  {
    return ::std::tgamma(x);
  }

Rval std_math_lgamma(Rval x)
  {
    return ::std::lgamma(x);
  }

void create_bindings_math(Oval& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.math.e`
    //===================================================================
    result.insert_or_assign(::rocket::sref("e"),
      Rval(
        // The base of the natural logarithm.
        s_const_e
      ));
    //===================================================================
    // `std.math.pi`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pi"),
      Rval(
        // The ratio of a circle's circumference to its diameter.
        s_const_pi
      ));
    //===================================================================
    // `std.math.lb10`
    //===================================================================
    result.insert_or_assign(::rocket::sref("real_max"),
      Rval(
        // The binary logarithm of the integer ten.
        s_const_lb10
      ));
    //===================================================================
    // `std.math.exp()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("exp"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.exp"));
    // Parse arguments.
    Rval y;
    Ropt base;
    if(reader.I().v(y).o(base).F()) {
      // Call the binding function.
      return std_math_exp(::rocket::move(y), ::rocket::move(base));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.exp(y, [base])`

  * Calculates `base` raised to the power `y`. If `base` is absent,
    `e` is assumed. This function is equivalent to `pow(base, y)`.

  * Returns the power as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.expm1()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("expm1"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.expm1"));
    // Parse arguments.
    Rval y;
    if(reader.I().v(y).F()) {
      // Call the binding function.
      return std_math_expm1(::rocket::move(y));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.expm1(y)`

  * Calculates `exp(y) - 1` without losing precision when `y` is
    close to zero.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.pow()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pow"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.pow"));
    // Parse arguments.
    Rval x;
    Rval y;
    if(reader.I().v(x).v(y).F()) {
      // Call the binding function.
      return std_math_pow(::rocket::move(x), ::rocket::move(y));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.pow(x, y)`

  * Calculates `x` raised to the power `y`. According to C99, when
    `x` is `1` or `y` is `0`, the result is always `1`, even when
    the other argument is an infinity or NaN.

  * Returns the power as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.log()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("log"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.log"));
    // Parse arguments.
    Rval y;
    Ropt base;
    if(reader.I().v(y).o(base).F()) {
      // Call the binding function.
      return std_math_log(::rocket::move(y), ::rocket::move(base));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.log(x, [base])`

  * Calculates the logarithm of `x` to `base`. If `base` is absent,
    `e` is assumed.

  * Returns the logarithm as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.log1p()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("log1p"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.log1p"));
    // Parse arguments.
    Rval y;
    if(reader.I().v(y).F()) {
      // Call the binding function.
      return std_math_log1p(::rocket::move(y));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.log1p(x)`

  * Calculates `log(1 + x)` without losing precision when `x` is
    close to zero.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.sin()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sin"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.sin"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_sin(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.sin(x)`

  * Calculates the sine of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.cos()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("cos"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.cos"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_cos(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.cos(x)`

  * Calculates the cosine of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.tan()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("tan"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.tan"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_tan(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.tan(x)`

  * Calculates the tangent of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.asin()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("asin"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.asin"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_asin(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.asin(x)`

  * Calculates the inverse sine of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.acos()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("acos"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.acos"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_acos(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.acos(x)`

  * Calculates the inverse cosine of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.atan()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("atan"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.atan"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_atan(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.atan(x)`

  * Calculates the inverse tangent of `x` in radians.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.atan2()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("atan2"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.atan2"));
    // Parse arguments.
    Rval y;
    Rval x;
    if(reader.I().v(y).v(x).F()) {
      // Call the binding function.
      return std_math_atan2(::rocket::move(y), ::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.atan2(y, x)`

  * Calculates the angle of the vector `<x,y>` in radians.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.hypot()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("hypot"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.hypot"));
    // Parse variadic arguments.
    cow_vector<Value> values;
    if(reader.I().F(values)) {
      // Call the binding function.
      return std_math_hypot(::rocket::move(values));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.hypot(...)`

  * Calculates the length of the n-dimension vector defined by all
    arguments. If no argument is provided, this function returns
    zero. If any argument is an infinity, this function returns a
    positive infinity; otherwise, if any argument is a NaN, this
    function returns a NaN.

  * Returns the length as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.sinh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sinh"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.sinh"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_sinh(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.sinh(x)`

  * Calculates the hyperbolic sine of `x`.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.cosh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("cosh"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.cosh"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_cosh(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.cosh(x)`

  * Calculates the hyperbolic cosine of `x`.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.tanh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("tanh"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.tanh"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_tanh(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.tanh(x)`

  * Calculates the hyperbolic tangent of `x`.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.asinh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("asinh"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.asinh"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_asinh(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.asinh(x)`

  * Calculates the inverse hyperbolic sine of `x`.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.acosh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("acosh"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.acosh"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_acosh(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.acosh(x)`

  * Calculates the inverse hyperbolic cosine of `x`.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.atanh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("atanh"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.atanh"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_atanh(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.atanh(x)`

  * Calculates the inverse hyperbolic tangent of `x`.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.erf()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("erf"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.erf"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_erf(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.erf(x)`

  * Calculates the error function of `x`.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.cerf()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("cerf"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.cerf"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_cerf(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.cerf(x)`

  * Calculates the complementary error function of `x`.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.gamma()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gamma"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.gamma"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_gamma(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.gamma(x)`

  * Calculates the Gamma function of `x`.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.math.lgamma()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("lgamma"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.math.lgamma"));
    // Parse arguments.
    Rval x;
    if(reader.I().v(x).F()) {
      // Call the binding function.
      return std_math_lgamma(::rocket::move(x));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.math.lgamma(x)`

  * Calculates the natural logarithm of the absolute value of the
    Gamma function of `x`.

  * Returns the result as a real.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // End of `std.math`
    //===================================================================
  }

}  // namespace Asteria
