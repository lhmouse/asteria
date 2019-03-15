// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "standard_bindings_debug.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

namespace Asteria {

bool std_debug_print(const Cow_Vector<Value> &values)
  {
    rocket::insertable_ostream mos;
    // Deal with nasty separators.
    auto cur = values.begin();
    auto rem = values.size();
    switch(rem) {
    default:
      while(--rem != 0) {
        cur->print(mos);
        mos << ' ';
        ++cur;
      }
      // Fallthrough.
    case 1:
      cur->print(mos);
      // Fallthrough.
    case 0:
      break;
    }
    bool succ = write_log_to_stderr(__FILE__, __LINE__, mos.extract_string());
    return succ;
  }

bool std_debug_dump(const Value &value, std::size_t indent_increment)
  {
    rocket::insertable_ostream mos;
    value.dump(mos, indent_increment);
    bool succ = write_log_to_stderr(__FILE__, __LINE__, mos.extract_string());
    return succ;
  }

D_object create_standard_bindings_debug()
  {
    D_object debug;
    //===================================================================
    // `std.debug.print()`
    //===================================================================
    debug.try_emplace(rocket::sref("print"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.debug.print([args...])`"
                     "\n  * Prints all `args` to the standard error stream, separated by"
                     "\n    spaces. A line break is appended to terminate the line."
                     "\n  * Returns `true` if the operation succeeds."),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.debug.print"), args);
            // Parse variadic arguments.
            Cow_Vector<Value> values(args.size());
            reader.start();
            std::for_each(values.mut_begin(), values.mut_end(), [&](Value &value) { reader.opt(value);  });
            if(reader.finish()) {
              // Call the binding function.
              if(!std_debug_print(values)) {
                // Fail.
                return Reference_Root::S_null();
              }
              // Return `true`.
              Reference_Root::S_temporary ref_c = { D_boolean(true) };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.debug.dump()`
    //===================================================================
    debug.try_emplace(rocket::sref("dump"),
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
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.debug.dump"), args);
            // Parse arguments.
            Value value;
            D_integer indent_increment = 2;
            if(reader.start().opt(value).opt(indent_increment).finish()) {
              // Clamp `indent_increment`.
              auto rindent = static_cast<std::size_t>(rocket::clamp(indent_increment, 0, 10));
              // Call the binding function.
              if(!std_debug_dump(value, rindent)) {
                // Fail.
                return Reference_Root::S_null();
              }
              // Return `true`.
              Reference_Root::S_temporary ref_c = { D_boolean(true) };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // End of `std.debug`
    //===================================================================
    return debug;
  }

}  // namespace Asteria
