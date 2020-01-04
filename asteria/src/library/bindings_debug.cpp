// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_debug.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

namespace Asteria {

bool std_debug_print(Sval templ, cow_vector<Value> values)
  {
    // Prepare inserters.
    cow_vector<::rocket::formatter> insts;
    insts.reserve(values.size());
    for(size_t i = 0; i != values.size(); ++i) {
      insts.push_back({
        [](tinyfmt& fmt, const void* ptr) -> tinyfmt&
          { return static_cast<const Value*>(ptr)->print(fmt);  },
        values.data() + i
      });
    }
    // Compose the string into a stream.
    ::rocket::tinyfmt_str fmt;
    vformat(fmt, templ.data(), templ.size(), insts.data(), insts.size());
    return write_log_to_stderr(__FILE__, __LINE__, fmt.extract_string());
  }

bool std_debug_dump(Value value, Iopt indent)
  {
    // Clamp the suggested indent so we don't produce overlong lines.
    size_t rindent = static_cast<size_t>(::rocket::clamp(indent.value_or(2), 0, 10));
    // Format the value.
    ::rocket::tinyfmt_str fmt;
    value.dump(fmt, rindent);
    return write_log_to_stderr(__FILE__, __LINE__, fmt.extract_string());
  }

void create_bindings_debug(Oval& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.debug.print()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("print"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.debug.print(...)`\n"
          "\n"
          "  * Compose a `string` in the same way as `std.string.format()`,\n"
          "    but instead of returning it, write it to standard error. A line\n"
          "    break is appended to terminate the line.\n"
          "\n"
          "  * Returns `true` if the operation succeeds, or `null` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.debug.print"), ::rocket::ref(args));
          // Parse variadic arguments.
          Sval templ;
          cow_vector<Value> values;
          if(reader.I().g(templ).F(values)) {
            // Call the binding function.
            if(!std_debug_print(templ, values)) {
              return Reference_Root::S_void();
            }
            Reference_Root::S_temporary xref = { true };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.debug.dump()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("dump"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.debug.dump(value, [indent])`\n"
          "\n"
          "  * Prints the value to standard error with detailed information.\n"
          "    `indent` specifies the number of spaces to use as a single\n"
          "    level of indent. Its value is clamped between `0` and `10`\n"
          "    inclusively. If it is set to `0`, no line break is inserted and\n"
          "    output lines are not indented. It has a default value of `2`.\n"
          "\n"
          "  * Returns `true` if the operation succeeds, or `null` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.debug.dump"), ::rocket::ref(args));
          // Parse arguments.
          Value value;
          Iopt indent;
          if(reader.I().g(value).g(indent).F()) {
            // Call the binding function.
            if(!std_debug_dump(value, indent)) {
              return Reference_Root::S_void();
            }
            Reference_Root::S_temporary xref = { true };
            return ::rocket::move(xref);
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
