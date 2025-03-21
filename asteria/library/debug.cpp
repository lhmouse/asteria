// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "debug.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../utils.hpp"
namespace asteria {

optV_integer
std_debug_logf(V_string templ, cow_vector<Value> values)
  {
    // Prepare inserters.
    cow_vector<::rocket::formatter> insts;
    insts.reserve(values.size());
    for(const auto& val : values)
      insts.push_back({
        [](tinyfmt& fmt, const void* ptr) {
          const auto& r = *(const Value*) ptr;
          if(r.is_string())
            fmt << r.as_string();
          else
            r.print_to(fmt);
        },
        &val });

    // Compose the string into a stream.
    ::rocket::tinyfmt_str fmt;
    vformat(fmt, templ.safe_c_str(), insts.data(), insts.size());
    ::ptrdiff_t nbytes = ASTERIA_WRITE_STDERR(fmt.extract_string());
    return (nbytes < 0) ? nullopt : optV_integer(nbytes);
  }

optV_integer
std_debug_dump(Value value, optV_integer indent)
  {
    // Clamp the suggested indent so we don't produce overlong lines.
    size_t rindent = ::rocket::clamp_cast<size_t>(indent.value_or(2), 0, 10);

    // Format the value.
    ::rocket::tinyfmt_str fmt;
    value.dump(fmt, rindent);
    ::ptrdiff_t nbytes = ASTERIA_WRITE_STDERR(fmt.extract_string());
    return (nbytes < 0) ? nullopt : optV_integer(nbytes);
  }

void
create_bindings_debug(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(&"logf",
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

    result.insert_or_assign(&"dump",
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
