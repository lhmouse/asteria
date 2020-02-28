// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_math.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/collector.hpp"
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
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.exp(y, [base])`\n"
          "\n"
          "  * Calculates `base` raised to the power `y`. If `base` is absent,\n"
          "    `e` is assumed. This function is equivalent to `pow(base, y)`.\n"
          "\n"
          "  * Returns the power as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.exp"), ::rocket::ref(args));
          // Parse arguments.
          Rval y;
          Ropt base;
          if(reader.I().g(y).g(base).F()) {
            // Call the binding function.
            return std_math_exp(::rocket::move(y), ::rocket::move(base));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.expm1()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("expm1"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.expm1(y)`\n"
          "\n"
          "  * Calculates `exp(y) - 1` without losing precision when `y` is\n"
          "    close to zero.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.expm1"), ::rocket::ref(args));
          // Parse arguments.
          Rval y;
          if(reader.I().g(y).F()) {
            // Call the binding function.
            return std_math_expm1(::rocket::move(y));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.pow()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pow"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.pow(x, y)`\n"
          "\n"
          "  * Calculates `x` raised to the power `y`. According to C99, when\n"
          "    `x` is `1` or `y` is `0`, the result is always `1`, even when\n"
          "    the other argument is an infinity or NaN.\n"
          "\n"
          "  * Returns the power as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.pow"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          Rval y;
          if(reader.I().g(x).g(y).F()) {
            // Call the binding function.
            return std_math_pow(::rocket::move(x), ::rocket::move(y));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.log()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("log"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.log(x, [base])`\n"
          "\n"
          "  * Calculates the logarithm of `x` to `base`. If `base` is absent,\n"
          "    `e` is assumed.\n"
          "\n"
          "  * Returns the logarithm as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.log"), ::rocket::ref(args));
          // Parse arguments.
          Rval y;
          Ropt base;
          if(reader.I().g(y).g(base).F()) {
            // Call the binding function.
            return std_math_log(::rocket::move(y), ::rocket::move(base));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.log1p()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("log1p"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.log1p(x)`\n"
          "\n"
          "  * Calculates `log(1 + x)` without losing precision when `x` is\n"
          "    close to zero.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.log1p"), ::rocket::ref(args));
          // Parse arguments.
          Rval y;
          if(reader.I().g(y).F()) {
            // Call the binding function.
            return std_math_log1p(::rocket::move(y));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.sin()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sin"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.sin(x)`\n"
          "\n"
          "  * Calculates the sine of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.sin"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_sin(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.cos()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("cos"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.cos(x)`\n"
          "\n"
          "  * Calculates the cosine of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.cos"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_cos(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.tan()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("tan"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.tan(x)`\n"
          "\n"
          "  * Calculates the tangent of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.tan"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_tan(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.asin()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("asin"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.asin(x)`\n"
          "\n"
          "  * Calculates the inverse sine of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.asin"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_asin(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.acos()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("acos"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.acos(x)`\n"
          "\n"
          "  * Calculates the inverse cosine of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.acos"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_acos(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.atan()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("atan"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.atan(x)`\n"
          "\n"
          "  * Calculates the inverse tangent of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.atan"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_atan(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.atan2()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("atan2"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.atan2(y, x)`\n"
          "\n"
          "  * Calculates the angle of the vector `<x,y>` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.atan2"), ::rocket::ref(args));
          // Parse arguments.
          Rval y;
          Rval x;
          if(reader.I().g(y).g(x).F()) {
            // Call the binding function.
            return std_math_atan2(::rocket::move(y), ::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.hypot()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("hypot"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.hypot(...)`\n"
          "\n"
          "  * Calculates the length of the n-dimension vector defined by all\n"
          "    arguments. If no argument is provided, this function returns\n"
          "    zero. If any argument is an infinity, this function returns a\n"
          "    positive infinity; otherwise, if any argument is a NaN, this\n"
          "    function returns a NaN.\n"
          "\n"
          "  * Returns the length as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.hypot"), ::rocket::ref(args));
          // Parse variadic arguments.
          cow_vector<Value> values;
          if(reader.I().F(values)) {
            // Call the binding function.
            return std_math_hypot(::rocket::move(values));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.sinh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sinh"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.sinh(x)`\n"
          "\n"
          "  * Calculates the hyperbolic sine of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.sinh"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_sinh(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.cosh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("cosh"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.cosh(x)`\n"
          "\n"
          "  * Calculates the hyperbolic cosine of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.cosh"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_cosh(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.tanh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("tanh"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.tanh(x)`\n"
          "\n"
          "  * Calculates the hyperbolic tangent of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.tanh"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_tanh(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.asinh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("asinh"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.asinh(x)`\n"
          "\n"
          "  * Calculates the inverse hyperbolic sine of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.asinh"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_asinh(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.acosh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("acosh"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.acosh(x)`\n"
          "\n"
          "  * Calculates the inverse hyperbolic cosine of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.acosh"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_acosh(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.atanh()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("atanh"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.atanh(x)`\n"
          "\n"
          "  * Calculates the inverse hyperbolic tangent of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.atanh"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_atanh(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.erf()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("erf"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.erf(x)`\n"
          "\n"
          "  * Calculates the error function of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.erf"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_erf(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.cerf()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("cerf"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.cerf(x)`\n"
          "\n"
          "  * Calculates the complementary error function of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.cerf"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_cerf(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.gamma()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("gamma"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.gamma(x)`\n"
          "\n"
          "  * Calculates the Gamma function of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.gamma"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_gamma(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.lgamma()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("lgamma"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.math.lgamma(x)`\n"
          "\n"
          "  * Calculates the natural logarithm of the absolute value of the\n"
          "    Gamma function of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.math.lgamma"), ::rocket::ref(args));
          // Parse arguments.
          Rval x;
          if(reader.I().g(x).F()) {
            // Call the binding function.
            return std_math_lgamma(::rocket::move(x));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // End of `std.math`
    //===================================================================
  }

}  // namespace Asteria
