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
    if(*base <= 1) {
      return NAN;
    }
    return std::log2(x) / std::log2(*base);
  }

G_real std_math_log1p(const G_real& x)
  {
    return std::log1p(x);
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
        rocket::sref
          (
            "\n"
            "`std.math.exp(y, [base])`\n"
            "  \n"
            "  * Calculates `base` raised to the power `y`. If `base` is absent,\n"
            "    `e` is assumed. This function is equivalent to `pow(base, y)`.\n"
            "  \n"
            "  * Returns the power as a `real`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
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
          }
      )));
    //===================================================================
    // `std.math.expm1()`
    //===================================================================
    result.insert_or_assign(rocket::sref("expm1"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.math.expm1(y)`\n"
            "  \n"
            "  * Calculates `exp(y) - 1` without losing precision when `y` is\n"
            "    close to zero.\n"
            "  \n"
            "  * Returns the result as a `real`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
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
          }
      )));
    //===================================================================
    // `std.math.pow()`
    //===================================================================
    result.insert_or_assign(rocket::sref("pow"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.math.pow(x, y)`\n"
            "  \n"
            "  * Calculates `x` raised to the power `y`. According to C99, when\n"
            "    `x` is `1` or `y` is `0`, the result is always `1`, even when\n"
            "    the other argument is an infinity or NaN.\n"
            "  \n"
            "  * Returns the power as a `real`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
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
          }
      )));
    //===================================================================
    // `std.math.log()`
    //===================================================================
    result.insert_or_assign(rocket::sref("log"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.math.log(x, [base])`\n"
            "  \n"
            "  * Calculates the logarithm of `x` to `base`. If `base` is absent,\n"
            "    `e` is assumed.\n"
            "  \n"
            "  * Returns the logarithm as a `real`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
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
          }
      )));
    //===================================================================
    // `std.math.log1p()`
    //===================================================================
    result.insert_or_assign(rocket::sref("log1p"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.math.log1p(x)`\n"
            "  \n"
            "  * Calculates `log(1 + x)` without losing precision when `x` is\n"
            "    close to zero.\n"
            "  \n"
            "  * Returns the result as a `real`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
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
          }
      )));
    //===================================================================
    // End of `std.math`
    //===================================================================
  }

}  // namespace Asteria
