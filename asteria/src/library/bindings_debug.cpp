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
    auto rpos = values.begin();
    if(rpos != values.end()) {
      for(;;) {
        rpos->print(mos);
        if(++rpos == values.end()) {
          break;
        }
        mos << ' ';
      }
    }
    bool succ = write_log_to_stderr(__FILE__, __LINE__, mos.extract_string());
    return succ;
  }

bool std_debug_dump(const Value& value, D_integer indent_increment)
  {
    rocket::insertable_ostream mos;
    value.dump(mos, static_cast<std::size_t>(rocket::clamp(indent_increment, 0, 10)));
    bool succ = write_log_to_stderr(__FILE__, __LINE__, mos.extract_string());
    return succ;
  }

D_object create_bindings_debug()
  {
    D_object ro;
    //===================================================================
    // `std.debug.print()`
    //===================================================================
    ro.try_emplace(rocket::sref("print"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.debug.print(...)`"
                     "\n  * Prints all arguments to the standard error stream, separated by"
                     "\n    spaces. A line break is appended to terminate the line."
                     "\n  * Returns `true` if the operation succeeds."),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.debug.print"), args);
            // Parse variadic arguments.
            Cow_Vector<Value> values;
            if(reader.start().finish(values)) {
              // Call the binding function.
              if(!std_debug_print(values)) {
                // Fail.
                return Reference_Root::S_null();
              }
              // Return `true`.
              Reference_Root::S_temporary ref_c = { true };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.debug.dump()`
    //===================================================================
    ro.try_emplace(rocket::sref("dump"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.debug.dump(value, [indent_increment])`"
                     "\n  * Prints the value to the standard error stream with detailed"
                     "\n    information. `indent_increment` specifies the number of spaces"
                     "\n    used as a single level of indent. Its value is clamped between"
                     "\n    `0` and `10` inclusively. If it is set to `0`, no line break is"
                     "\n    inserted and output lines are not indented. It has a default"
                     "\n    value of `2`."
                     "\n  * Returns `true` if the operation succeeds."),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.debug.dump"), args);
            // Parse arguments.
            Value value;
            D_integer indent_increment = 2;
            if(reader.start().opt(value).opt(indent_increment).finish()) {
              // Call the binding function.
              if(!std_debug_dump(value, indent_increment)) {
                // Fail.
                return Reference_Root::S_null();
              }
              // Return `true`.
              Reference_Root::S_temporary ref_c = { true };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // End of `std.debug`
    //===================================================================
    return ro;
  }

}  // namespace Asteria
