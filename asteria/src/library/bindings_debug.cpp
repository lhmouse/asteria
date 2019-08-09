// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_debug.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

namespace Asteria {

bool std_debug_print(const cow_vector<Value>& values)
  {
    cow_osstream fmtss;
    fmtss.imbue(std::locale::classic());
    rocket::for_each(values, std::bind(&Value::print, std::placeholders::_1, std::ref(fmtss), false));
    bool succ = write_log_to_stderr(__FILE__, __LINE__, fmtss.extract_string());
    return succ;
  }

bool std_debug_dump(const Value& value, const opt<G_integer>& indent)
  {
    cow_osstream fmtss;
    fmtss.imbue(std::locale::classic());
    value.dump(fmtss, static_cast<size_t>(rocket::clamp(indent.value_or(2), 0, 10)));
    bool succ = write_log_to_stderr(__FILE__, __LINE__, fmtss.extract_string());
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
        rocket::sref(
          "\n"
          "`std.debug.print(...)`\n"
          "\n"
          "  * Prints all arguments to the standard error stream. A line break\n"
          "    is appended to terminate the line.\n"
          "\n"
          "  * Returns `true` if the operation succeeds, or `null` otherwise.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.debug.print"), rocket::ref(args));
          // Parse variadic arguments.
          cow_vector<Value> values;
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
        })
      ));
    //===================================================================
    // `std.debug.dump()`
    //===================================================================
    result.insert_or_assign(rocket::sref("dump"),
      G_function(make_simple_binding(
        // Description
        rocket::sref(
          "\n"
          "`std.debug.dump(value, [indent])`\n"
          "\n"
          "  * Prints the value to the standard error stream with detailed\n"
          "    information. `indent` specifies the number of spaces to use as\n"
          "    a single level of indent. Its value is clamped between `0` and\n"
          "    `10` inclusively. If it is set to `0`, no line break is\n"
          "    inserted and output lines are not indented. It has a default\n"
          "    value of `2`.\n"
          "\n"
          "  * Returns `true` if the operation succeeds, or `null` otherwise.\n"
        ),
        // Opaque parameter
        G_null(
          nullptr
        ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(rocket::sref("std.debug.dump"), rocket::ref(args));
          // Parse arguments.
          Value value;
          opt<G_integer> indent;
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
        })
      ));
    //===================================================================
    // End of `std.debug`
    //===================================================================
  }

}  // namespace Asteria
