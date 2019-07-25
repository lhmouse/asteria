// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_math.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/collector.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    constexpr G_real s_const_e    = 2.7182818284590452353602874713526624977572470937000;
    constexpr G_real s_const_pi   = 3.1415926535897932384626433832795028841971693993751;
    constexpr G_real s_const_lb10 = 3.3219280948873623478703194294893901758648313930246;

    }

G_real std_math_exp(const G_real& y, const Opt<G_real>& base)
  {
    if(!base) {
      return std::exp(y);
    }
    if(*base == 2) {
      return std::exp2(y);
    }
    if(*base == s_const_e) {
      return std::exp(y);
    }
    return std::pow(*base, y);
  }

G_real std_math_expm1(const G_real& y)
  {
    return std::expm1(y);
  }

G_real std_math_pow(const G_real& x, const G_real& y)
  {
    return std::pow(x, y);
  }

G_real std_math_log(const G_real& x, const Opt<G_real>& base)
  {
    if(!base) {
      return std::log(x);
    }
    if(*base == 2) {
      return std::log2(x);
    }
    if(*base == 10) {
      return std::log10(x);
    }
    if((*base == 1) || (*base <= 0)) {
      return NAN;
    }
    return std::log2(x) / std::log2(*base);
  }

G_real std_math_log1p(const G_real& x)
  {
    return std::log1p(x);
  }

G_real std_math_sin(const G_real& x)
  {
    return std::sin(x);
  }

G_real std_math_cos(const G_real& x)
  {
    return std::cos(x);
  }

G_real std_math_tan(const G_real& x)
  {
    return std::tan(x);
  }

G_real std_math_asin(const G_real& x)
  {
    return std::asin(x);
  }

G_real std_math_acos(const G_real& x)
  {
    return std::acos(x);
  }

G_real std_math_atan(const G_real& x)
  {
    return std::atan(x);
  }

G_real std_math_atan2(const G_real& y, const G_real& x)
  {
    return std::atan2(y, x);
  }

G_real std_math_hypot(const Cow_Vector<Value>& values)
  {
    switch(values.size()) {
    case 0:
      {
        // Return zero if no argument is provided.
        return 0;
      }
    case 1:
      {
        // Return the absolute value of the only argument.
        return std::fabs(values[0].convert_to_real());
      }
    default:
      {
        // Call the C `hypot()` function.
        auto result = std::hypot(values[0].convert_to_real(), values[1].convert_to_real());
        // Sum up all the other values.
        for(std::size_t i = 2; i < values.size(); ++i) {
          result = std::hypot(result, values[i].convert_to_real());
        }
        return result;
      }
    }
  }

G_real std_math_sinh(const G_real& x)
  {
    return std::sinh(x);
  }

G_real std_math_cosh(const G_real& x)
  {
    return std::cosh(x);
  }

G_real std_math_tanh(const G_real& x)
  {
    return std::tanh(x);
  }

G_real std_math_asinh(const G_real& x)
  {
    return std::asinh(x);
  }

G_real std_math_acosh(const G_real& x)
  {
    return std::acosh(x);
  }

G_real std_math_atanh(const G_real& x)
  {
    return std::atanh(x);
  }

G_real std_math_erf(const G_real& x)
  {
    return std::erf(x);
  }

G_real std_math_cerf(const G_real& x)
  {
    return std::erfc(x);
  }

G_real std_math_gamma(const G_real& x)
  {
    return std::tgamma(x);
  }

G_real std_math_lgamma(const G_real& x)
  {
    return std::lgamma(x);
  }

void create_bindings_math(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.math.e`
    //===================================================================
    result.insert_or_assign(rocket::sref("e"),
      G_real(
        // The base of the natural logarithm.
        s_const_e
      ));
    //===================================================================
    // `std.math.pi`
    //===================================================================
    result.insert_or_assign(rocket::sref("pi"),
      G_real(
        // The ratio of a circle's circumference to its diameter.
        s_const_pi
      ));
    //===================================================================
    // `std.math.lb10`
    //===================================================================
    result.insert_or_assign(rocket::sref("real_max"),
      G_real(
        // The binary logarithm of the integer ten.
        s_const_lb10
      ));
    //===================================================================
    // `std.math.exp()`
    //===================================================================
    result.insert_or_assign(rocket::sref("exp"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.exp(y, [base])`\n"
          "\n"
          "  * Calculates `base` raised to the power `y`. If `base` is absent,\n"
          "    `e` is assumed. This function is equivalent to `pow(base, y)`.\n"
          "\n"
          "  * Returns the power as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.exp"), args);
          // Parse arguments.
          G_real y;
          Opt<G_real> base;
          if(reader.start().g(y).g(base).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_exp(y, base) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.expm1()`
    //===================================================================
    result.insert_or_assign(rocket::sref("expm1"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.expm1(y)`\n"
          "\n"
          "  * Calculates `exp(y) - 1` without losing precision when `y` is\n"
          "    close to zero.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.expm1"), args);
          // Parse arguments.
          G_real y;
          if(reader.start().g(y).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_expm1(y) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.pow()`
    //===================================================================
    result.insert_or_assign(rocket::sref("pow"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.pow(x, y)`\n"
          "\n"
          "  * Calculates `x` raised to the power `y`. According to C99, when\n"
          "    `x` is `1` or `y` is `0`, the result is always `1`, even when\n"
          "    the other argument is an infinity or NaN.\n"
          "\n"
          "  * Returns the power as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.pow"), args);
          // Parse arguments.
          G_real x;
          G_real y;
          if(reader.start().g(x).g(y).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_pow(x, y) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.log()`
    //===================================================================
    result.insert_or_assign(rocket::sref("log"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.log(x, [base])`\n"
          "\n"
          "  * Calculates the logarithm of `x` to `base`. If `base` is absent,\n"
          "    `e` is assumed.\n"
          "\n"
          "  * Returns the logarithm as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.log"), args);
          // Parse arguments.
          G_real y;
          Opt<G_real> base;
          if(reader.start().g(y).g(base).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_log(y, base) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.log1p()`
    //===================================================================
    result.insert_or_assign(rocket::sref("log1p"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.log1p(x)`\n"
          "\n"
          "  * Calculates `log(1 + x)` without losing precision when `x` is\n"
          "    close to zero.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.log1p"), args);
          // Parse arguments.
          G_real y;
          if(reader.start().g(y).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_log1p(y) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.sin()`
    //===================================================================
    result.insert_or_assign(rocket::sref("sin"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.sin(x)`\n"
          "\n"
          "  * Calculates the sine of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.sin"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_sin(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.cos()`
    //===================================================================
    result.insert_or_assign(rocket::sref("cos"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.cos(x)`\n"
          "\n"
          "  * Calculates the cosine of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.cos"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_cos(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.tan()`
    //===================================================================
    result.insert_or_assign(rocket::sref("tan"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.tan(x)`\n"
          "\n"
          "  * Calculates the tangent of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.tan"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_tan(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.asin()`
    //===================================================================
    result.insert_or_assign(rocket::sref("asin"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.asin(x)`\n"
          "\n"
          "  * Calculates the inverse sine of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.asin"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_asin(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.acos()`
    //===================================================================
    result.insert_or_assign(rocket::sref("acos"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.acos(x)`\n"
          "\n"
          "  * Calculates the inverse cosine of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.acos"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_acos(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.atan()`
    //===================================================================
    result.insert_or_assign(rocket::sref("atan"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.atan(x)`\n"
          "\n"
          "  * Calculates the inverse tangent of `x` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.atan"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_atan(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.atan2()`
    //===================================================================
    result.insert_or_assign(rocket::sref("atan2"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.atan2(y, x)`\n"
          "\n"
          "  * Calculates the angle of the vector `<x,y>` in radians.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.atan2"), args);
          // Parse arguments.
          G_real y;
          G_real x;
          if(reader.start().g(y).g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_atan2(y, x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.hypot()`
    //===================================================================
    result.insert_or_assign(rocket::sref("hypot"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
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
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.hypot"), args);
          // Parse variadic arguments.
          Cow_Vector<Value> values;
          if(reader.start().finish(values)) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_hypot(values) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.sinh()`
    //===================================================================
    result.insert_or_assign(rocket::sref("sinh"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.sinh(x)`\n"
          "\n"
          "  * Calculates the hyperbolic sine of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.sinh"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_sinh(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.cosh()`
    //===================================================================
    result.insert_or_assign(rocket::sref("cosh"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.cosh(x)`\n"
          "\n"
          "  * Calculates the hyperbolic cosine of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.cosh"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_cosh(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.tanh()`
    //===================================================================
    result.insert_or_assign(rocket::sref("tanh"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.tanh(x)`\n"
          "\n"
          "  * Calculates the hyperbolic tangent of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.tanh"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_tanh(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.asinh()`
    //===================================================================
    result.insert_or_assign(rocket::sref("asinh"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.asinh(x)`\n"
          "\n"
          "  * Calculates the inverse hyperbolic sine of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.asinh"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_asinh(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.acosh()`
    //===================================================================
    result.insert_or_assign(rocket::sref("acosh"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.acosh(x)`\n"
          "\n"
          "  * Calculates the inverse hyperbolic cosine of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.acosh"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_acosh(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.atanh()`
    //===================================================================
    result.insert_or_assign(rocket::sref("atanh"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.atanh(x)`\n"
          "\n"
          "  * Calculates the inverse hyperbolic tangent of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.atanh"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_atanh(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.erf()`
    //===================================================================
    result.insert_or_assign(rocket::sref("erf"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.erf(x)`\n"
          "\n"
          "  * Calculates the error function of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.erf"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_erf(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.cerf()`
    //===================================================================
    result.insert_or_assign(rocket::sref("cerf"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.cerf(x)`\n"
          "\n"
          "  * Calculates the complementary error function of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.cerf"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_cerf(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.gamma()`
    //===================================================================
    result.insert_or_assign(rocket::sref("gamma"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.gamma(x)`\n"
          "\n"
          "  * Calculates the Gamma function of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.gamma"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_gamma(x) };
            return rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.math.lgamma()`
    //===================================================================
    result.insert_or_assign(rocket::sref("lgamma"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.math.lgamma(x)`\n"
          "\n"
          "  * Calculates the natural logarithm of the absolute value of the\n"
          "    Gamma function of `x`.\n"
          "\n"
          "  * Returns the result as a `real`.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.math.lgamma"), args);
          // Parse arguments.
          G_real x;
          if(reader.start().g(x).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_math_lgamma(x) };
            return rocket::move(xref);
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
