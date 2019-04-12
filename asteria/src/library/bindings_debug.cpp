// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_debug.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

namespace Asteria {

bool std_debug_print(const Cow_Vector<Value>& values)
  {
    rocket::insertable_ostream mos;
    rocket::for_each(values, [&](const Value& value) { mos << value;  });
    bool succ = write_log_to_stderr(__FILE__, __LINE__, mos.extract_string());
    return succ;
  }

bool std_debug_dump(const Value& value, const Opt<G_integer>& indent)
  {
    rocket::insertable_ostream mos;
    value.dump(mos, static_cast<std::size_t>(rocket::clamp(indent.value_or(2), 0, 10)));
    bool succ = write_log_to_stderr(__FILE__, __LINE__, mos.extract_string());
    return succ;
  }

void create_bindings_debug(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.debug.print()`
    //===================================================================
    result.insert_or_assign(rocket::sref("print"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.debug.print(...)`\n"
                     "  * Prints all arguments to the standard error stream. A line break\n"
                     "    is appended to terminate the line.\n"
                     "  * Returns `true` if the operation succeeds.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.debug.print"), args);
            // Parse variadic arguments.
            Cow_Vector<Value> values;
            if(reader.start().finish(values)) {
              // Call the binding function.
              if(!std_debug_print(values)) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { true };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // `std.debug.dump()`
    //===================================================================
    result.insert_or_assign(rocket::sref("dump"),
      G_function(make_simple_binding(
        // Description
        rocket::sref("`std.debug.dump(value, [indent])`\n"
                     "  * Prints the value to the standard error stream with detailed\n"
                     "    information. `indent` specifies the number of spaces to use as\n"
                     "    a single level of indent. Its value is clamped between `0` and\n"
                     "    `10` inclusively. If it is set to `0`, no line break is\n"
                     "    inserted and output lines are not indented. It has a default\n"
                     "    value of `2`.\n"
                     "  * Returns `true` if the operation succeeds.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.debug.dump"), args);
            // Parse arguments.
            Value value;
            Opt<G_integer> indent;
            if(reader.start().g(value).g(indent).finish()) {
              // Call the binding function.
              if(!std_debug_dump(value, indent)) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { true };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        G_null()
      )));
    //===================================================================
    // End of `std.debug`
    //===================================================================
  }

}  // namespace Asteria
