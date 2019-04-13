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

G_integer std_numeric_abs(const G_integer& value)
  {
    if(value == INT64_MIN) {
      ASTERIA_THROW_RUNTIME_ERROR("The absolute value of `", value, "` cannot be represented as an `integer`.");
    }
    return std::abs(value);
  }

G_real std_numeric_abs(const G_real& value)
  {
    return std::fabs(value);
  }

G_integer std_numeric_sign(const G_integer& value)
  {
    return value >> 63;
  }

G_integer std_numeric_sign(const G_real& value)
  {
    return static_cast<std::int64_t>(std::signbit(value) == 0) - 1;
  }

G_boolean std_numeric_is_finite(const G_integer& /*value*/)
  {
    return true;
  }

G_boolean std_numeric_is_finite(const G_real& value)
  {
    return std::isfinite(value);
  }

G_boolean std_numeric_is_infinity(const G_integer& /*value*/)
  {
    return false;
  }

G_boolean std_numeric_is_infinity(const G_real& value)
  {
    return std::isinf(value);
  }

G_boolean std_numeric_is_nan(const G_integer& /*value*/)
  {
    return false;
  }

G_boolean std_numeric_is_nan(const G_real& value)
  {
    return std::isnan(value);
  }

G_integer std_numeric_clamp(const G_integer& value, const G_integer& lower, const G_integer& upper)
  {
    if(!(lower <= upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` limit must be less than or equal to the `upper` limit (got `", lower, "` and `", upper, "`).");
    }
    return rocket::clamp(value, lower, upper);
  }

G_real std_numeric_clamp(const G_real& value, const G_real& lower, const G_real& upper)
  {
    if(!std::islessequal(lower, upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` limit must be less than or equal to the `upper` limit (got `", lower, "` and `", upper, "`).");
    }
    return rocket::clamp(value, lower, upper);
  }

G_integer std_numeric_round(const G_integer& value)
  {
    return value;
  }

G_real std_numeric_round(const G_real& value)
  {
    return std::round(value);
  }

G_integer std_numeric_floor(const G_integer& value)
  {
    return value;
  }

G_real std_numeric_floor(const G_real& value)
  {
    return std::floor(value);
  }

G_integer std_numeric_ceil(const G_integer& value)
  {
    return value;
  }

G_real std_numeric_ceil(const G_real& value)
  {
    return std::ceil(value);
  }

G_integer std_numeric_trunc(const G_integer& value)
  {
    return value;
  }

G_real std_numeric_trunc(const G_real& value)
  {
    return std::trunc(value);
  }

    namespace {

    G_integer do_icast(const G_real& value)
      {
        if((value < INT64_MIN) || (value > (INT64_MAX & -0x400))) {
          ASTERIA_THROW_RUNTIME_ERROR("The `real` number `", value, "` cannot be represented as an `integer`.");
        }
        return G_integer(value);
      }

    }

G_integer std_numeric_iround(const G_integer& value)
  {
    return value;
  }

G_integer std_numeric_iround(const G_real& value)
  {
    return do_icast(std::round(value));
  }

G_integer std_numeric_ifloor(const G_integer& value)
  {
    return value;
  }

G_integer std_numeric_ifloor(const G_real& value)
  {
    return do_icast(std::floor(value));
  }

G_integer std_numeric_iceil(const G_integer& value)
  {
    return value;
  }

G_integer std_numeric_iceil(const G_real& value)
  {
    return do_icast(std::ceil(value));
  }

G_integer std_numeric_itrunc(const G_integer& value)
  {
    return value;
  }

G_integer std_numeric_itrunc(const G_real& value)
  {
    return do_icast(std::trunc(value));
  }

    namespace {

    double do_random_ratio(const Global_Context& global) noexcept
      {
        // sqword <= [0,INT64_MAX]
        std::int64_t sqword = global.get_random_uint32();
        sqword <<= 31;
        sqword ^= global.get_random_uint32();
        // return <= [0,1)
        return static_cast<double>(sqword) / 0x1p63;
      }

    }

G_integer std_numeric_random(const Global_Context& global, const G_integer& upper)
  {
    if(!(0 < upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `upper` limit must be greater than zero (got `", upper, "`).");
    }
    auto res = do_random_ratio(global);
    res *= static_cast<double>(upper);
    return static_cast<std::int64_t>(res);
  }

G_real std_numeric_random(const Global_Context& global, const Opt<G_real>& upper)
  {
    if(upper && !std::isless(0, *upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `upper` limit must be greater than zero (got `", *upper, "`).");
    }
    auto res = do_random_ratio(global);
    if(upper) {
      res *= *upper;
    }
    return res;
  }

G_integer std_numeric_random(const Global_Context& global, const G_integer& lower, const G_integer& upper)
  {
    if(!(lower < upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` limit must be less than the `upper` limit (got `", lower, "` and `", upper, "`).");
    }
    auto res = do_random_ratio(global);
    res *= static_cast<double>(upper - lower);
    return lower + static_cast<std::int64_t>(res);
  }

G_real std_numeric_random(const Global_Context& global, const G_real& lower, const G_real& upper)
  {
    if(!std::isless(lower, upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` limit must be less than the `upper` limit (got `", lower, "` and `", upper, "`).");
    }
    auto res = do_random_ratio(global);
    res *= upper - lower;
    return lower + res;
  }

G_real std_numeric_sqrt(const G_real& x)
  {
    return std::sqrt(x);
  }

G_real std_numeric_fma(const G_real& x, const G_real& y, const G_real& z)
  {
    return std::fma(x, y, z);
  }

G_integer std_numeric_addm(const G_integer& x, const G_integer& y)
  {
    return G_integer(static_cast<std::uint64_t>(x) + static_cast<std::uint64_t>(y));
  }

G_integer std_numeric_subm(const G_integer& x, const G_integer& y)
  {
    return G_integer(static_cast<std::uint64_t>(x) - static_cast<std::uint64_t>(y));
  }

G_integer std_numeric_mulm(const G_integer& x, const G_integer& y)
  {
    return G_integer(static_cast<std::uint64_t>(x) * static_cast<std::uint64_t>(y));
  }

    namespace {

    ROCKET_PURE_FUNCTION G_integer do_saturing_add(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs >= 0) {
          if(lhs > INT64_MAX - rhs) {
            return INT64_MAX;
          }
        } else {
          if(lhs < INT64_MIN - rhs) {
            return INT64_MIN;
          }
        }
        return lhs + rhs;
      }
    ROCKET_PURE_FUNCTION G_integer do_saturing_sub(const G_integer& lhs, const G_integer& rhs)
      {
        if(rhs >= 0) {
          if(lhs < INT64_MIN + rhs) {
            return INT64_MIN;
          }
        } else {
          if(lhs > INT64_MAX + rhs) {
            return INT64_MAX;
          }
        }
        return lhs - rhs;
      }
    ROCKET_PURE_FUNCTION G_integer do_saturing_mul(const G_integer& lhs, const G_integer& rhs)
      {
        if((lhs == 0) || (rhs == 0)) {
          return 0;
        }
        if((lhs == 1) || (rhs == 1)) {
          return lhs ^ rhs ^ 1;
        }
        if((lhs == INT64_MIN) || (rhs == INT64_MIN)) {
          return (lhs >> 63) ^ (rhs >> 63) ^ INT64_MAX;
        }
        if((lhs == -1) || (rhs == -1)) {
          return -(lhs ^ rhs ^ -1);
        }
        // signed lhs and absolute rhs
        auto slhs = lhs;
        auto arhs = rhs;
        if(rhs < 0) {
          slhs = -lhs;
          arhs = -rhs;
        }
        if(slhs >= 0) {
          if(slhs > INT64_MAX / arhs) {
            return INT64_MAX;
          }
        } else {
          if(slhs < INT64_MIN / arhs) {
            return INT64_MIN;
          }
        }
        return slhs * arhs;
      }

    ROCKET_PURE_FUNCTION G_real do_saturing_add(const G_real& lhs, const G_real& rhs)
      {
        return lhs + rhs;
      }
    ROCKET_PURE_FUNCTION G_real do_saturing_sub(const G_real& lhs, const G_real& rhs)
      {
        return lhs - rhs;
      }
    ROCKET_PURE_FUNCTION G_real do_saturing_mul(const G_real& lhs, const G_real& rhs)
      {
        return lhs * rhs;
      }

    }

G_integer std_numeric_adds(const G_integer& x, const G_integer& y)
  {
    return do_saturing_add(x, y);
  }

G_real std_numeric_adds(const G_real& x, const G_real& y)
  {
    return do_saturing_add(x, y);
  }

G_integer std_numeric_adds(const G_integer& x, const G_integer& y, const G_integer& lower, const G_integer& upper)
  {
    if(!(lower < upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` limit must be less than the `upper` limit (got `", lower, "` and `", upper, "`).");
    }
    return rocket::clamp(do_saturing_add(x, y), lower, upper);
  }

G_real std_numeric_adds(const G_real& x, const G_real& y, const G_real& lower, const G_real& upper)
  {
    if(!std::isless(lower, upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` limit must be less than the `upper` limit (got `", lower, "` and `", upper, "`).");
    }
    return rocket::clamp(do_saturing_add(x, y), lower, upper);
  }

G_integer std_numeric_subs(const G_integer& x, const G_integer& y)
  {
    return do_saturing_sub(x, y);
  }

G_real std_numeric_subs(const G_real& x, const G_real& y)
  {
    return do_saturing_sub(x, y);
  }

G_integer std_numeric_subs(const G_integer& x, const G_integer& y, const G_integer& lower, const G_integer& upper)
  {
    if(!(lower < upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` limit must be less than the `upper` limit (got `", lower, "` and `", upper, "`).");
    }
    return rocket::clamp(do_saturing_sub(x, y), lower, upper);
  }

G_real std_numeric_subs(const G_real& x, const G_real& y, const G_real& lower, const G_real& upper)
  {
    if(!std::isless(lower, upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` limit must be less than the `upper` limit (got `", lower, "` and `", upper, "`).");
    }
    return rocket::clamp(do_saturing_sub(x, y), lower, upper);
  }

G_integer std_numeric_muls(const G_integer& x, const G_integer& y)
  {
    return do_saturing_mul(x, y);
  }

G_real std_numeric_muls(const G_real& x, const G_real& y)
  {
    return do_saturing_mul(x, y);
  }

G_integer std_numeric_muls(const G_integer& x, const G_integer& y, const G_integer& lower, const G_integer& upper)
  {
    if(!(lower < upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` limit must be less than the `upper` limit (got `", lower, "` and `", upper, "`).");
    }
    return rocket::clamp(do_saturing_mul(x, y), lower, upper);
  }

G_real std_numeric_muls(const G_real& x, const G_real& y, const G_real& lower, const G_real& upper)
  {
    if(!std::isless(lower, upper)) {
      ASTERIA_THROW_RUNTIME_ERROR("The `lower` limit must be less than the `upper` limit (got `", lower, "` and `", upper, "`).");
    }
    return rocket::clamp(do_saturing_mul(x, y), lower, upper);
  }

void create_bindings_numeric(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.numeric.abs()`
    //===================================================================
    result.insert_or_assign(rocket::sref("abs"),
      G_function(make_simple_binding(
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
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_abs(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_abs(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.sign()`
    //===================================================================
    result.insert_or_assign(rocket::sref("sign"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.sign(value)`\n"
                     "  * Propagates the sign bit of the number `value`, which may be an\n"
                     "    `integer` or `real`, to all bits of an `integer`. Be advised\n"
                     "    that `-0.0` is distinct from `0.0` despite the equality.\n"
                     "  * Returns `-1` if `value` is negative and `0` otherwise.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.sign"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_sign(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_sign(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.is_finite()`
    //===================================================================
    result.insert_or_assign(rocket::sref("is_finite"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.is_finite(value)`\n"
                     "  * Checks whether `value` is a finite number. `value` may be an\n"
                     "    `integer` or `real`. Be adviced that this functions returns\n"
                     "    `true` for `integer`s for consistency; `integer`s do not\n"
                     "    support infinities or NaNs.\n"
                     "  * Returns `true` if `value` is an `integer`, or is a `real` that\n"
                     "    is neither an infinity or a NaN; otherwise `false`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.is_finite"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_finite(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_finite(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.is_infinity()`
    //===================================================================
    result.insert_or_assign(rocket::sref("is_infinity"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.is_infinity(value)`\n"
                     "  * Checks whether `value` is an infinity. `value` may be an\n"
                     "    `integer` or `real`. Be adviced that this functions returns\n"
                     "    `false` for `integer`s for consistency; `integer`s do not\n"
                     "    support infinities.\n"
                     "  * Returns `true` if `value` is a `real` that denotes an infinity;\n"
                     "    otherwise `false`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.is_infinity"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_infinity(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_infinity(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.is_nan()`
    //===================================================================
    result.insert_or_assign(rocket::sref("is_nan"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.is_nan(value)`\n"
                     "  * Checks whether `value` is a NaN. `value` may be an `integer` or\n"
                     "    `real`. Be adviced that this functions returns `false` for\n"
                     "    `integer`s for consistency; `integer`s do not support NaNs.\n"
                     "  * Returns `true` if `value` is a `real` denoting a NaN; otherwise\n"
                     "    `false`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.is_nan"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_nan(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_is_nan(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.clamp()`
    //===================================================================
    result.insert_or_assign(rocket::sref("clamp"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.clamp(value, lower, upper)`\n"
                     "  * Limits `value` between `lower` and `upper`.\n"
                     "  * Returns `lower` if `value < lower`, `upper` if `value > upper`,\n"
                     "    and `value` otherwise (including when `value` is a NaN). The\n"
                     "    returned value is of type `integer` if all arguments are of\n"
                     "    type `integer`; otherwise it is of type `real`.\n"
                     "  * Throws an exception if `lower` is not less than or equal to\n"
                     "    `upper`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.clamp"), args);
            // Parse arguments.
            G_integer ivalue;
            G_integer ilower;
            G_integer iupper;
            if(reader.start().g(ivalue).g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_clamp(ivalue, ilower, iupper) };
              return rocket::move(xref);
            }
            G_real rvalue;
            G_real flower;
            G_real fupper;
            if(reader.start().g(rvalue).g(flower).g(fupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_clamp(rvalue, flower, fupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.round()`
    //===================================================================
    result.insert_or_assign(rocket::sref("round"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.round(value)`\n"
                     "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
                     "    nearest integer; halfway values are rounded away from zero. If\n"
                     "    `value` is an `integer`, it is returned intact.\n"
                     "  * Returns the rounded value.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.round"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_round(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_round(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.floor()`
    //===================================================================
    result.insert_or_assign(rocket::sref("floor"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.floor(value)`\n"
                     "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
                     "    nearest integer towards negative infinity. If `value` is an\n"
                     "    `integer`, it is returned intact.\n"
                     "  * Returns the rounded value.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.floor"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_floor(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_floor(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.ceil()`
    //===================================================================
    result.insert_or_assign(rocket::sref("ceil"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.ceil(value)`\n"
                     "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
                     "    nearest integer towards positive infinity. If `value` is an\n"
                     "    `integer`, it is returned intact.\n"
                     "  * Returns the rounded value.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.ceil"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_ceil(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_ceil(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.trunc()`
    //===================================================================
    result.insert_or_assign(rocket::sref("trunc"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.trunc(value)`\n"
                     "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
                     "    nearest integer towards zero. If `value` is an `integer`, it is\n"
                     "    returned intact.\n"
                     "  * Returns the rounded value.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.trunc"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_trunc(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_trunc(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.iround()`
    //===================================================================
    result.insert_or_assign(rocket::sref("iround"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.iround(value)`\n"
                     "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
                     "    nearest integer; halfway values are rounded away from zero. If\n"
                     "    `value` is an `integer`, it is returned intact. The result is\n"
                     "    converted to an `integer`.\n"
                     "  * Returns the rounded value as an `integer`.\n"
                     "  * Throws an exception if the result cannot be represented as an\n"
                     "    `integer`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.iround"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_iround(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_iround(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.ifloor()`
    //===================================================================
    result.insert_or_assign(rocket::sref("ifloor"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.ifloor(value)`\n"
                     "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
                     "    nearest integer towards negative infinity. If `value` is an\n"
                     "    `integer`, it is returned intact. The result is converted to\n"
                     "    an `integer`.\n"
                     "  * Returns the rounded value as an `integer`.\n"
                     "  * Throws an exception if the result cannot be represented as an\n"
                     "    `integer`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.ifloor"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_ifloor(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_ifloor(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.iceil()`
    //===================================================================
    result.insert_or_assign(rocket::sref("iceil"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.iceil(value)`\n"
                     "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
                     "    nearest integer towards positive infinity. If `value` is an\n"
                     "    `integer`, it is returned intact. The result is converted to\n"
                     "    an `integer`.\n"
                     "  * Returns the rounded value as an `integer`.\n"
                     "  * Throws an exception if the result cannot be represented as an\n"
                     "    `integer`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.iceil"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_iceil(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_iceil(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.itrunc()`
    //===================================================================
    result.insert_or_assign(rocket::sref("itrunc"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.itrunc(value)`\n"
                     "  * Rounds `value`, which may be an `integer` or `real`, to the\n"
                     "    nearest integer towards zero. If `value` is an `integer`, it is\n"
                     "    returned intact. The result is converted to an `integer`.\n"
                     "  * Returns the rounded value as an `integer`.\n"
                     "  * Throws an exception if the result cannot be represented as an\n"
                     "    `integer`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.itrunc"), args);
            // Parse arguments.
            G_integer ivalue;
            if(reader.start().g(ivalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_itrunc(ivalue) };
              return rocket::move(xref);
            }
            G_real rvalue;
            if(reader.start().g(rvalue).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_itrunc(rvalue) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.random()`
    //===================================================================
    result.insert_or_assign(rocket::sref("random"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.random([upper])`\n"
                     "  * Generates a random `integer` or `real` that is less than\n"
                     "    `upper`. If `upper` is absent, it has a default value of `1.0`\n"
                     "    which is a `real`.\n"
                     "  * Returns a non-negative `integer` or `real` that is less than\n"
                     "    `upper`. The return value is of type `integer` if `upper` is of\n"
                     "    type `integer`; otherwise it is of type `real`.\n"
                     "  * Throws an exception if `upper` is negative or zero.\n"
                     "`std.numeric.random(lower, upper)`\n"
                     "  * Generates a random `integer` or `real` that is not less than\n"
                     "    `lower` but is less than `upper`. `lower` and `upper` shall be\n"
                     "    of the same type.\n"
                     "  * Returns an `integer` or `real` that is not less than `lower`\n"
                     "    but is less than `upper`. The return value is of type `integer`\n"
                     "    if both arguments are of type `integer`; otherwise it is of\n"
                     "    type `real`.\n"
                     "  * Throws an exception if `lower` is not less than `upper`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.random"), args);
            // Parse arguments.
            G_integer iupper;
            if(reader.start().g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(global, iupper) };
              return rocket::move(xref);
            }
            Opt<G_real> kupper;
            if(reader.start().g(kupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(global, kupper) };
              return rocket::move(xref);
            }
            G_integer ilower;
            if(reader.start().g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(global, ilower, iupper) };
              return rocket::move(xref);
            }
            G_real fupper;
            G_real flower;
            if(reader.start().g(flower).g(fupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_random(global, flower, fupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.sqrt()`
    //===================================================================
    result.insert_or_assign(rocket::sref("sqrt"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.sqrt(x)`\n"
                     "  * Calculates the square root of `x` which may be of either the\n"
                     "    `integer` or the `real` type. The result is always exact.\n"
                     "  * Returns the square root of `x` as a `real`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.sqrt"), args);
            // Parse arguments.
            G_real x;
            if(reader.start().g(x).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_sqrt(x) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.fma()`
    //===================================================================
    result.insert_or_assign(rocket::sref("fma"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.fma(x, y, z)`\n"
                     "  * Performs fused multiply-add operation on `x`, `y` and `z`. This\n"
                     "    functions calculates `x * y + z` without intermediate rounding\n"
                     "    operations. The result is always exact.\n"
                     "  * Returns the value of `x * y + z` as a `real`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.fma"), args);
            // Parse arguments.
            G_real x;
            G_real y;
            G_real z;
            if(reader.start().g(x).g(y).g(z).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_fma(x, y, z) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.addm()`
    //===================================================================
    result.insert_or_assign(rocket::sref("addm"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.addm(x, y)`\n"
                     "  * Adds `y` to `x` using modular arithmetic. `x` and `y` must be\n"
                     "    of the `integer` type. The result is reduced to be congruent to\n"
                     "    the sum of `x` and `y` modulo `0x1p64` in infinite precision.\n"
                     "    This function will not cause overflow exceptions to be thrown.\n"
                     "  * Returns the reduced sum of `x` and `y`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.addm"), args);
            // Parse arguments.
            G_integer x;
            G_integer y;
            if(reader.start().g(x).g(y).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_addm(x, y) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.subm()`
    //===================================================================
    result.insert_or_assign(rocket::sref("subm"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.subm(x, y)`\n"
                     "  * Subtracts `y` from `x` using modular arithmetic. `x` and `y`\n"
                     "    must be of the `integer` type. The result is reduced to be\n"
                     "    congruent to the difference of `x` and `y` modulo `0x1p64` in\n"
                     "    infinite precision. This function will not cause overflow\n"
                     "    exceptions to be thrown.\n"
                     "  * Returns the reduced difference of `x` and `y`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.subm"), args);
            // Parse arguments.
            G_integer x;
            G_integer y;
            if(reader.start().g(x).g(y).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subm(x, y) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.mulm()`
    //===================================================================
    result.insert_or_assign(rocket::sref("mulm"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.mulm(x, y)`\n"
                     "  * Multiplies `x` by `y` using modular arithmetic. `x` and `y`\n"
                     "    must be of the `integer` type. The result is reduced to be\n"
                     "    congruent to the product of `x` and `y` modulo `0x1p64` in\n"
                     "    infinite precision. This function will not cause overflow\n"
                     "    exceptions to be thrown.\n"
                     "  * Returns the reduced product of `x` and `y`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.mulm"), args);
            // Parse arguments.
            G_integer x;
            G_integer y;
            if(reader.start().g(x).g(y).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_mulm(x, y) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.adds()`
    //===================================================================
    result.insert_or_assign(rocket::sref("adds"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.adds(x, y)`\n"
                     "  * Adds `y` to `x` using saturating arithmetic. `x` and `y` may be\n"
                     "    `integer` or `real` values. The result is limited within the\n"
                     "    range of representable values of its type, hence will not cause\n"
                     "    overflow exceptions to be thrown. When either argument is of\n"
                     "    type `real` which supports infinities, this function is\n"
                     "    equivalent to the built-in addition operator.\n"
                     "  * Returns the saturated sum of `x` and `y`.\n"
                     "`std.numeric.adds(x, y, lower, upper)`\n"
                     "  * Adds `y` to `x` using saturating arithmetic. `x` and `y` may be\n"
                     "    `integer` or `real` values. The result is limited between\n"
                     "    `lower` and `upper`, hence will not cause overflow exceptions\n"
                     "    to be thrown.\n"
                     "  * Returns the saturated sum of `x` and `y`. The result is of type\n"
                     "    `integer` if all arguments are of type `integer`; otherwise it\n"
                     "    is of type `real`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.adds"), args);
            Argument_Reader::State istate, fstate;
            // Parse arguments.
            G_integer ix;
            G_integer iy;
            if(reader.start().g(ix).g(iy).save_state(istate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_adds(ix, iy) };
              return rocket::move(xref);
            }
            G_real fx;
            G_real fy;
            if(reader.start().g(fx).g(fy).save_state(fstate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_adds(fx, fy) };
              return rocket::move(xref);
            }
            G_integer ilower;
            G_integer iupper;
            if(reader.load_state(istate).g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_adds(ix, iy, ilower, iupper) };
              return rocket::move(xref);
            }
            G_real flower;
            G_real fupper;
            if(reader.load_state(fstate).g(flower).g(fupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_adds(fx, fy, flower, fupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.subs()`
    //===================================================================
    result.insert_or_assign(rocket::sref("subs"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.subs(x, y)`\n"
                     "  * Subtracts `y` from `x` using saturating arithmetic. `x` and `y`\n"
                     "    may be `integer` or `real` values. The result is limited within\n"
                     "    the range of representable values of its type, hence will not\n"
                     "    cause overflow exceptions to be thrown. When either argument is\n"
                     "    of type `real` which supports infinities, this function is\n"
                     "    equivalent to the built-in subtraction operator.\n"
                     "  * Returns the saturated difference of `x` and `y`.\n"
                     "`std.numeric.subs(x, y, lower, upper)`\n"
                     "  * Subtracts `y` from `x` using saturating arithmetic. `x` and `y`\n"
                     "    may be `integer` or `real` values. The result is limited\n"
                     "    between `lower` and `upper`, hence will not cause overflow\n"
                     "    exceptions to be thrown.\n"
                     "  * Returns the saturated difference of `x` and `y`. The result is\n"
                     "    of type `integer` if all arguments are of type `integer`;\n"
                     "    otherwise it is of type `real`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.subs"), args);
            Argument_Reader::State istate, fstate;
            // Parse arguments.
            G_integer ix;
            G_integer iy;
            if(reader.start().g(ix).g(iy).save_state(istate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subs(ix, iy) };
              return rocket::move(xref);
            }
            G_real fx;
            G_real fy;
            if(reader.start().g(fx).g(fy).save_state(fstate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subs(fx, fy) };
              return rocket::move(xref);
            }
            G_integer ilower;
            G_integer iupper;
            if(reader.load_state(istate).g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subs(ix, iy, ilower, iupper) };
              return rocket::move(xref);
            }
            G_real flower;
            G_real fupper;
            if(reader.load_state(fstate).g(flower).g(fupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_subs(fx, fy, flower, fupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.numeric.muls()`
    //===================================================================
    result.insert_or_assign(rocket::sref("muls"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.numeric.muls(x, y)`\n"
                     "  * Multiplies `x` by `y` using saturating arithmetic. `x` and `y`\n"
                     "    may be `integer` or `real` values. The result is limited within\n"
                     "    the range of representable values of its type, hence will not\n"
                     "    cause overflow exceptions to be thrown. When either argument is\n"
                     "    of type `real` which supports infinities, this function is\n"
                     "    equivalent to the built-in multiplication operator.\n"
                     "  * Returns the saturated product of `x` and `y`.\n"
                     "`std.numeric.muls(x, y, lower, upper)`\n"
                     "  * Multiplies `x` by `y` using saturating arithmetic. `x` and `y`\n"
                     "    may be `integer` or `real` values. The result is limited\n"
                     "    between `lower` and `upper`, hence will not cause overflow\n"
                     "    exceptions to be thrown.\n"
                     "  * Returns the saturated product of `x` and `y`. The result is of\n"
                     "    type `integer` if all arguments are of type `integer`;\n"
                     "    otherwise it is of type `real`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.numeric.muls"), args);
            Argument_Reader::State istate, fstate;
            // Parse arguments.
            G_integer ix;
            G_integer iy;
            if(reader.start().g(ix).g(iy).save_state(istate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_muls(ix, iy) };
              return rocket::move(xref);
            }
            G_real fx;
            G_real fy;
            if(reader.start().g(fx).g(fy).save_state(fstate).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_muls(fx, fy) };
              return rocket::move(xref);
            }
            G_integer ilower;
            G_integer iupper;
            if(reader.load_state(istate).g(ilower).g(iupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_muls(ix, iy, ilower, iupper) };
              return rocket::move(xref);
            }
            G_real flower;
            G_real fupper;
            if(reader.load_state(fstate).g(flower).g(fupper).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_numeric_muls(fx, fy, flower, fupper) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // End of `std.numeric`
    //===================================================================
  }

}  // namespace Asteria
