// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_numeric.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/global_context.hpp"
#include "../runtime/collector.hpp"
#include "../utilities.hpp"

namespace Asteria {

D_integer std_numeric_abs(const D_integer& value)
  {
    if(value == INT64_MIN) {
      ASTERIA_THROW_RUNTIME_ERROR("The absolute value of `", value, "` cannot be represented as an `integer`.");
    }
    return std::abs(value);
  }

D_real std_numeric_abs(const D_real& value)
  {
    return std::fabs(value);
  }

D_integer std_numeric_signbit(const D_integer& value)
  {
    return value >> 63;
  }

D_integer std_numeric_signbit(const D_real& value)
  {
    bool b = std::signbit(value);
    return -static_cast<std::int64_t>(b);
  }

    namespace {

    inline D_real do_rcast(const D_integer& value)
      {
        return D_real(value);
      }
    inline D_real do_rcast(const D_real& value)
      {
        if(std::fpclassify(value) == FP_NAN) {
          ASTERIA_THROW_RUNTIME_ERROR("The value `", value, "` is unordered with everything.");
        }
        return value;
      }

    }

D_integer std_numeric_clamp(const D_integer& value, const D_integer& lower, const D_integer& upper)
  {
    return rocket::clamp(value, lower, upper);
  }

D_real std_numeric_clamp(const D_integer& value, const D_real& lower, const D_real& upper)
  {
    return rocket::clamp(do_rcast(value), do_rcast(lower), do_rcast(upper));
  }

D_real std_numeric_clamp(const D_real& value, const D_integer& lower, const D_integer& upper)
  {
    return rocket::clamp(do_rcast(value), do_rcast(lower), do_rcast(upper));
  }

D_real std_numeric_clamp(const D_real& value, const D_real& lower, const D_real& upper)
  {
    return rocket::clamp(do_rcast(value), do_rcast(lower), do_rcast(upper));
  }

D_real std_numeric_random()
  {
    return 0.5;
  }

D_integer std_numeric_random(const D_integer& upper)
  {
    return upper / 2;
  }

D_real std_numeric_random(const D_real& upper)
  {
    return upper / 2;
  }

D_integer std_numeric_random(const D_integer& lower, const D_integer& upper)
  {
    return (lower + upper) / 2;
  }

D_real std_numeric_random(const D_real& lower, const D_real& upper)
  {
    return (lower + upper) / 2;
  }

void create_bindings_numeric(D_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.numeric.abs()`
    //===================================================================
    result.insert_or_assign(rocket::sref("abs"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.abs(value)`\n"
                     "  * Gets the absolute value of `value`, which may be an `integer`\n"
                     "    or `real`. Negative `integer`s are negated, which might cause\n"
                     "    an exception to be thrown due to overflow. Sign bits of `real`s\n"
                     "    are removed, which works on infinities and NaNs and does not\n"
                     "    result in exceptions.\n"
                     "  * Return the absolute value.\n"
                     "  * Throws an exception if `value` is the `integer` `-0x1p63`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.abs"), args);
            // Parse arguments.
            D_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_abs(ivalue) };
              return rocket::move(xref);
            }
            D_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_abs(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.numeric.signbit()`
    //===================================================================
    result.insert_or_assign(rocket::sref("signbit"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.signbit(value)`\n"
                     "  * Populates the sign bit of the number `value`, which may be an\n"
                     "    `integer` or `real`, to all bits of an `integer`. Be advised\n"
                     "    that `-0.0` is distinct from `0.0` despite the equality.\n"
                     "  * Returns `-1` if `value` is negative and `0` otherwise.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.signbit"), args);
            // Parse arguments.
            D_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_signbit(ivalue) };
              return rocket::move(xref);
            }
            D_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_signbit(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.numeric.clamp()`
    //===================================================================
    result.insert_or_assign(rocket::sref("clamp"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.clamp(value, lower, upper)`\n"
                     "  * Limits `value` between `lower` and `upper`. `lower` and `upper`\n"
                     "    shall be of the same type. The type of value returned by this\n"
                     "    function depends on its arguments.\n"
                     "  * Returns `lower` if `value < lower`, `upper` if `value > upper`,\n"
                     "    and `value` otherwise.\n"
                     "  * Throws an exception if `value` is unordered with `lower` or\n"
                     "    `upper`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.clamp"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_integer ivalue;
            D_integer ilower;
            D_integer iupper;
            if(reader.start().g(ivalue).save_state(state).g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_clamp(ivalue, ilower, iupper) };
              return rocket::move(xref);
            }
            D_real rlower;
            D_real rupper;
            if(reader.load_state(state).g(rlower).g(rupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_clamp(ivalue, rlower, rupper) };
              return rocket::move(xref);
            }
            D_real rvalue;
            if(reader.start().g(rvalue).save_state(state).g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_clamp(rvalue, ilower, iupper) };
              return rocket::move(xref);
            }
            if(reader.load_state(state).g(rlower).g(rupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_clamp(rvalue, rlower, rupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.numeric.random()`
    //===================================================================
    result.insert_or_assign(rocket::sref("random"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.random()`\n"
                     "  * Generates a random `real`.\n"
                     "  * Returns a non-negative `real` that is less than `1.0`.\n"
                     "`std.numeric.random(upper)`\n"
                     "  * Generates a random `integer` or `real` that is less than\n"
                     "    `upper`. The type of value returned by this function depends\n"
                     "    on its argument.\n"
                     "  * Returns a non-negative `integer` or `real` that is less than\n"
                     "    `upper`.\n"
                     "  * Throws an exception if `upper` is negative or zero.\n"
                     "`std.numeric.random(lower, upper)`\n"
                     "  * Generates a random `integer` or `real` that is not less than\n"
                     "    `lower` but is less than `upper`. `lower` and `upper` shall be\n"
                     "    of the same type. The type of value returned by this function\n"
                     "    depends on its arguments.\n"
                     "  * Returns an `integer` or `real` that is not less than `lower`\n"
                     "    but is less than `upper`.\n"
                     "  * Throws an exception if `lower` is not less than `upper`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.random"), args);
            Argument_Reader::State state;
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random() };
              return rocket::move(xref);
            }
            D_integer ilower;
            if(reader.start().g(ilower).save_state(state).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(ilower) };
              return rocket::move(xref);
            }
            D_integer iupper;
            if(reader.load_state(state).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(ilower, iupper) };
              return rocket::move(xref);
            }
            D_real rlower;
            if(reader.start().g(rlower).save_state(state).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(rlower) };
              return rocket::move(xref);
            }
            D_real rupper;
            if(reader.load_state(state).g(rupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(rlower, rupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // End of `std.numeric`
    //===================================================================
  }

}  // namespace Asteria
