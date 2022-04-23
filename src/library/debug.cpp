// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "debug.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/runtime_error.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

void
do_print_value(tinyfmt& fmt, const void* ptr)
  {
    static_cast<const Value*>(ptr)->print(fmt);
  }

optV_integer
do_write_stderr_common(::rocket::tinyfmt_str&& fmt)
  {
    // Try writing standard output. Errors are ignored.
    auto nput = write_log_to_stderr(__FILE__, __LINE__, fmt.extract_string());
    if(nput < 0)
      return nullopt;

    // Return the number of bytes that have been written.
    return static_cast<int64_t>(nput);
  }

}  // namespace

optV_integer
std_debug_logf(V_string templ, cow_vector<Value> values)
  {
    // Prepare inserters.
    cow_vector<::rocket::formatter> insts;
    insts.reserve(values.size());
    for(const auto& val : values)
      insts.push_back({ do_print_value, &val });

    // Compose the string into a stream.
    ::rocket::tinyfmt_str fmt;
    vformat(fmt, templ.data(), templ.size(), insts.data(), insts.size());
    return do_write_stderr_common(::std::move(fmt));
  }

optV_integer
std_debug_dump(Value value, optV_integer indent)
  {
    // Clamp the suggested indent so we don't produce overlong lines.
    size_t rindent = ::rocket::clamp_cast<size_t>(indent.value_or(2), 0, 10);

    // Format the value.
    ::rocket::tinyfmt_str fmt;
    value.dump(fmt, rindent);
    return do_write_stderr_common(::std::move(fmt));
  }

void
create_bindings_debug(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("logf"),
      ASTERIA_BINDING(
        "std.debug.logf", "templ, ...",
        Argument_Reader&& reader)
      {
        V_string templ;
        cow_vector<Value> values;

        reader.start_overload();
        reader.required(templ);
        if(reader.end_overload(values))
          return (Value) std_debug_logf(templ, values);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("dump"),
      ASTERIA_BINDING(
        "std.debug.dump", "[value], [indent]",
        Argument_Reader&& reader)
      {
        Value value;
        optV_integer indent;

        reader.start_overload();
        reader.optional(value);
        reader.optional(indent);
        if(reader.end_overload())
          return (Value) std_debug_dump(value, indent);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
