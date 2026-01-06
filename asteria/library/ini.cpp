// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "ini.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../utils.hpp"
namespace asteria {
namespace {

constexpr char s_reject[]   = "[]=;#";
constexpr char s_comment[]  = ";#";
constexpr char s_space[]    = " \t";

tinyfmt&
do_format_key(tinyfmt& fmt, const cow_string& key)
  {
    if(key.empty())
      ASTERIA_THROW(("Empty key is not allowed"));

    if(key.find_of(s_reject) != cow_string::npos)
      ASTERIA_THROW(("Key contains invalid characters: $1"), key);

    if(key.find_of(s_space) == 0)
      ASTERIA_THROW(("Key shall not begin with a space: $1"), key);

    if(key.rfind_of(s_space) == key.size() - 1)
      ASTERIA_THROW(("Key shall not end with a space: $1"), key);

    return fmt << key;
  }

bool
do_format_check_scalar(const Value& value)
  {
    switch(value.type())
      {
      case type_null:
      case type_boolean:
      case type_integer:
      case type_real:
        // These values are always convertible to strings.
        return true;

      case type_string:
        {
          // Verify the string, as we will have to write it verbatim later.
          const auto& str = value.as_string();
          if(str.empty())
            return true;

          if(str.find_of(s_reject) != cow_string::npos)
            ASTERIA_THROW(("Value contains invalid characters: $1"), str);

          if(str.find_of(s_space) == 0)
            ASTERIA_THROW(("Value shall not begin with a space: $1"), str);

          if(str.rfind_of(s_space) == str.size() - 1)
            ASTERIA_THROW(("Value shall not end with a space: $1"), str);

          return true;
        }

      case type_opaque:
      case type_function:
      case type_array:
      case type_object:
        // These values are always ignored.
        return false;

      default:
        ASTERIA_TERMINATE(("Corrupted enumeration `$1`"), value.type());
      }
  }

void
do_format_to(::rocket::tinyfmt& fmt, const V_object& value)
  {
    size_t nlines = 0;

    for(const auto& r : value)
      if(do_format_check_scalar(r.second)) {
        // Write a top-level property.
        do_format_key(fmt, r.first);
        fmt << "=";

        if(r.second.is_string())
          fmt << r.second.as_string();
        else
          r.second.print_to(fmt);

        fmt << "\r\n";
        nlines ++;
      }

    for(const auto& ro : value)
      if(ro.second.is_object()) {
        if(nlines != 0)
          fmt << "\r\n";

        // Write a section header.
        fmt << "[";
        do_format_key(fmt, ro.first);
        fmt << "]\r\n";
        nlines ++;

        // Write all properties in this section.
        for(const auto& r : ro.second.as_object())
          if(do_format_check_scalar(r.second)) {
            // Write a key-value pair.
            do_format_key(fmt, r.first);
            fmt << "=";

            if(r.second.is_string())
              fmt << r.second.as_string();
            else
              r.second.print_to(fmt);

            fmt << "\r\n";
            nlines ++;
          }
      }
  }

void
do_parse_from(V_object& root, tinyfmt& fmt)
  {
    V_object* sink = &root;

    // Read source file in lines.
    cow_string line;
    size_t nlines = 0;
    while(getline(line, fmt)) {
      nlines ++;

      // Remove the UTF-8 BOM, if any.
      if((nlines == 1) && line.starts_with("\xEF\xBB\xBF", 3))
        line.erase(0, 3);

      // Convert CR LF pairs to LF characters.
      if(line.ends_with("\r"))
        line.pop_back();

      // Remove comments.
      size_t pos = line.find_of(s_comment);
      if(pos != cow_string::npos)
        line.erase(pos);

      // Remove leading and trailing spaces.
      // Empty lines are ignored.
      pos = line.find_not_of(s_space);
      if(pos == cow_string::npos)
        continue;

      line.erase(0, pos);
      pos = line.rfind_not_of(s_space);
      line.erase(pos + 1);

      cow_string key = line;
      cow_string value;

      // if the line begins with an open bracket, it shall start a section.
      if(line.front() == '[') {
        if(line.back() != ']')
          ASTERIA_THROW(("Invalid section name on line $1"), nlines);

        // Trim the section name.
        pos = line.find_not_of(1, s_space);
        if(pos == line.size() - 1)
          ASTERIA_THROW(("Empty section name on line $1"), nlines);

        // Make a copy of the section name that is just large enough.
        key.assign(line, 1, line.size() - 2);

        // Insert a new section.
        auto& sub = root.try_emplace(move(key), V_object()).first->second;
        ROCKET_ASSERT(sub.is_object());
        sink = &(sub.open_object());
        continue;
      }

      // Otherwise, it shall be a property.
      size_t eqpos = line.find('=');
      if(eqpos != cow_string::npos) {
        pos = line.rfind_not_of(eqpos - 1, s_space);
        if(pos == cow_string::npos)
          ASTERIA_THROW(("Empty property name on line $1"), nlines);

        // Make copies of the key and value that are just large enough.
        key.assign(line, 0, pos + 1);

        // Get the value without leading spaces.
        pos = line.find_not_of(eqpos + 1, s_space);
        if(pos != cow_string::npos)
          value.assign(line, pos, line.size());
      }

      // Insert a new value.
      sink->insert_or_assign(move(key), move(value));
    }
  }

}  // namespace

V_string
std_ini_format(V_object value)
  {
    ::rocket::tinyfmt_str fmt;
    do_format_to(fmt, value);
    return fmt.extract_string();
  }

void
std_ini_format_to_file(V_string path, V_object value)
  {
    ::rocket::tinyfmt_file fmt;
    fmt.open(path.safe_c_str(), tinyfmt::open_write | tinyfmt::open_binary);
    do_format_to(fmt, value);
    fmt.flush();
  }

V_object
std_ini_parse(V_string text)
  {
    V_object value;
    ::rocket::tinyfmt_str fmt;
    fmt.set_string(text, tinyfmt::open_read);
    do_parse_from(value, fmt);
    return value;
  }

V_object
std_ini_parse_file(V_string path)
  {
    V_object value;
    ::rocket::tinyfmt_file fmt;
    fmt.open(path.safe_c_str(), tinyfmt::open_read | tinyfmt::open_binary);
    do_parse_from(value, fmt);
    return value;
  }

void
create_bindings_ini(V_object& result, API_Version version)
  {
    result.insert_or_assign(&"format",
      ASTERIA_BINDING(
        "std.ini.format", "object",
        Argument_Reader&& reader)
      {
        V_object object;

        reader.start_overload();
        reader.required(object);
        if(reader.end_overload())
          return (Value) std_ini_format(object);

        reader.throw_no_matching_function_call();
      });

    if(version >= api_version_0002_0000)
      result.insert_or_assign(&"format_to_file",
        ASTERIA_BINDING(
          "std.ini.format_to_file", "path, object",
          Argument_Reader&& reader)
        {
          V_string path;
          V_object object;

          reader.start_overload();
          reader.required(path);
          reader.required(object);
          if(reader.end_overload())
            return (void) std_ini_format_to_file(path, object);

          reader.throw_no_matching_function_call();
        });

    result.insert_or_assign(&"parse",
      ASTERIA_BINDING(
        "std.ini.parse", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_ini_parse(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"parse_file",
      ASTERIA_BINDING(
        "std.ini.parse_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_ini_parse_file(path);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
