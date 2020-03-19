// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "debug.hpp"
#include "../runtime/argument_reader.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

tinyfmt& do_print_value(tinyfmt& fmt, const void* ptr)
  {
    return static_cast<const Value*>(ptr)->print(fmt);
  }

}  // namespace

Iopt std_debug_print(Sval templ, cow_vector<Value> values)
  {
    // Prepare inserters.
    cow_vector<::rocket::formatter> insts;
    insts.reserve(values.size());
    for(size_t i = 0;  i < values.size();  ++i) {
      insts.push_back({ do_print_value, values.data() + i });
    }
    // Compose the string into a stream.
    ::rocket::tinyfmt_str fmt;
    vformat(fmt, templ.data(), templ.size(), insts.data(), insts.size());
    auto nput = write_log_to_stderr(__FILE__, __LINE__, fmt.extract_string());
    if(nput < 0) {
      return nullopt;
    }
    return static_cast<int64_t>(nput);
  }

Iopt std_debug_dump(Value value, Iopt indent)
  {
    // Clamp the suggested indent so we don't produce overlong lines.
    size_t rindent = static_cast<size_t>(::rocket::clamp(indent.value_or(2), 0, 10));
    // Format the value.
    ::rocket::tinyfmt_str fmt;
    value.dump(fmt, rindent);
    auto nput = write_log_to_stderr(__FILE__, __LINE__, fmt.extract_string());
    if(nput < 0) {
      return nullopt;
    }
    return static_cast<int64_t>(nput);
  }

void create_bindings_debug(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.debug.print()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("print"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.debug.print"));
    // Parse variadic arguments.
    Sval templ;
    cow_vector<Value> values;
    if(reader.I().v(templ).F(values)) {
      // Call the binding function.
      return std_debug_print(templ, values);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.debug.print(templ, ...)`

  * Compose a string in the same way as `std.string.format()`, but
    instead of returning it, write it to standard error. A line
    break is appended to terminate the line.

  * Returns the number of bytes written if the operation succeeds,
    or `null` otherwise.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.debug.dump()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("dump"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.debug.dump"));
    // Parse arguments.
    Value value;
    Iopt indent;
    if(reader.I().o(value).o(indent).F()) {
      // Call the binding function.
      return std_debug_dump(value, indent);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.debug.dump(value, [indent])`

  * Prints the value to standard error with detailed information.
    `indent` specifies the number of spaces to use as a single
    level of indent. Its value is clamped between `0` and `10`
    inclusively. If it is set to `0`, no line break is inserted and
    output lines are not indented. It has a default value of `2`.

  * Returns the number of bytes written if the operation succeeds,
    or `null` otherwise.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // End of `std.debug`
    //===================================================================
  }

}  // namespace Asteria
