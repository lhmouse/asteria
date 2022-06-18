// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "csv.hpp"
#include "../runtime/argument_reader.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

bool
do_format_check_scalar(const Value& value)
  {
    switch(value.type()) {
      case type_null:
      case type_boolean:
      case type_integer:
      case type_real:
      case type_string:
        // These values are always convertible to strings.
        return true;

      case type_opaque:
      case type_function:
      case type_array:
      case type_object:
        // These values are always ignored.
        return false;

      default:
        ASTERIA_TERMINATE((
            "Invalid value type (type `$1`)"),
            value.type());
    }
  }

V_array
do_csv_parse(tinybuf& buf)
  {
    V_array root;
    V_string* cell = nullptr;
    bool quote_allowed = true;
    size_t quote_at_line = 0;

    // Read source file in lines.
    cow_string line;
    size_t nlines = 0;
    while(getline(line, buf)) {
      nlines ++;

      // Remove the UTF-8 BOM, if any.
      if((nlines == 1) && line.starts_with("\xEF\xBB\xBF", 3))
        line.erase(0, 3);

      // Convert CR LF pairs to LF characters.
      if(line.ends_with(sref("\r")))
        line.pop_back();

      // Start a new line if this is not in quotation.
      if(quote_at_line == 0)
        root.emplace_back(V_array());

      // Parse the current line.
      size_t offset = 0;
      while(offset < line.size()) {
        if(quote_at_line == 0) {
          auto& row = root.mut_back().mut_array();

          if(row.empty()) {
            // We are not in quotation mode, so any character must start a value.
            row.emplace_back(V_string());
            cell = &(row.mut_back().mut_string());
          }

          if(quote_allowed && (line[offset] == '\"')) {
            // Enter quotation mode to append text to `cell`.
            offset ++;
            quote_at_line = nlines;
            continue;
          }

          // Search for the next comma.
          size_t epos = line.find(offset, ',');
          if(epos == cow_string::npos) {
            // Accept all the remaining characters.
            cell->append(line, offset);
            quote_allowed = true;
            break;
          }

          // Accept all characters between them and re-enable quotation mode.
          cell->append(line, offset, epos - offset);
          offset = epos + 1;
          quote_allowed = true;

          // Create a new cell after the current one.
          row.emplace_back(V_string());
          cell = &(row.mut_back().mut_string());
          continue;
        }

        // Search for the closing double quotation mark.
        size_t epos = line.find(offset, '\"');
        if(epos == cow_string::npos) {
          // Accept all the remaining characters, as well as the line break.
          cell->append(line, offset);
          cell->push_back('\n');
          break;
        }

        size_t dpos = epos + 1;
        if(line[dpos] == '\"') {
          // If the quotation mark is doubled (escaped), append the first one
          // and skip the other.
          cell->append(line, offset, dpos - offset);
          offset = dpos + 1;
          continue;
        }

        // Accept all characters between them and disable quotation mode.
        cell->append(line, offset, epos - offset);
        offset = epos + 1;
        quote_allowed = false;
        quote_at_line = 0;
      }
    }

    if(quote_at_line != 0)
      ASTERIA_THROW_RUNTIME_ERROR(("Unmatched \" at line $1"), quote_at_line);

    return root;
  }

}  // namespace

V_string
std_csv_format(V_array value)
  {
    ::rocket::tinyfmt_str fmt;

    for(auto rowp = value.begin();  rowp != value.end();  ++rowp) {
      const auto& row = rowp->as_array();

      // Write columns, separated by commas.
      for(auto cellp = row.begin();  cellp != row.end();  ++cellp) {
        if(cellp != row.begin())
          fmt << ",";

        if(!do_format_check_scalar(*cellp))
          continue;

        if(cellp->is_string()) {
          // Check whether we need to escape special characters.
          const auto& str = cellp->as_string();
          if(str.find_first_of(sref(",\n\"")) != cow_string::npos) {
            fmt << "\"";

            for(char c : str)
              if(c == '\"')
                fmt << "\"\"";
              else
                fmt << c;

            fmt << "\"";
            continue;
          }
        }

        // Write the value as is.
        fmt << *cellp;
      }

      fmt << "\r\n";
    }

    return fmt.extract_string();
  }

V_array
std_csv_parse(V_string text)
  {
    // Parse characters from the string.
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(text, tinybuf::open_read);
    return do_csv_parse(cbuf);
  }

V_array
std_csv_parse_file(V_string path)
  {
    // Try opening the file.
    ::rocket::unique_posix_file fp(::fopen(path.safe_c_str(), "rb"));
    if(!fp)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Could not open file '$2'",
          "[`fopen()` failed: $1]"),
          format_errno(), path);

    // Parse characters from the file.
    ::rocket::tinybuf_file cbuf(::std::move(fp));
    return do_csv_parse(cbuf);
  }

void
create_bindings_csv(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("format"),
      ASTERIA_BINDING(
        "std.csv.format", "[array]",
        Argument_Reader&& reader)
      {
        V_array array;

        reader.start_overload();
        reader.required(array);
        if(reader.end_overload())
          return (Value) std_csv_format(array);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("parse"),
      ASTERIA_BINDING(
        "std.csv.parse", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_csv_parse(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("parse_file"),
      ASTERIA_BINDING(
        "std.csv.parse_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_csv_parse_file(path);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
