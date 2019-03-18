// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_string.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

namespace Asteria {

int std_string_compare(const Cow_String &text_one, const Cow_String &text_two, std::size_t length)
  {
    return ROCKET_UNEXPECT(length < PTRDIFF_MAX) ? text_one.compare(0, length, text_two, 0, length)
                                                 : text_one.compare(text_two);
  }

Cow_String std_string_reverse(const Cow_String &text)
  {
    return Cow_String(text.rbegin(), text.rend());
  }

Cow_String std_string_trim(const Cow_String &text, const Cow_String &reject)
  {
    if(reject.empty()) {
      // There is no character to strip. Make use of reference counting.
      return text;
    }
    auto start = text.find_first_not_of(reject);
    if(start == Cow_String::npos) {
      // There is no character to keep. Return an empty string.
      return rocket::sref("");
    }
    auto end = text.find_last_not_of(reject);
    if((start == 0) && (end == text.size() - 1)) {
      // There is no character to strip. Make use of reference counting.
      return text;
    }
    // Return the remaining part of `text`.
    return text.substr(start, end + 1 - start);
  }

Cow_String std_string_trim_left(const Cow_String &text, const Cow_String &reject)
  {
    if(reject.empty()) {
      // There is no character to strip. Make use of reference counting.
      return text;
    }
    auto start = text.find_first_not_of(reject);
    if(start == Cow_String::npos) {
      // There is no character to keep. Return an empty string.
      return rocket::sref("");
    }
    if(start == 0) {
      // There is no character to strip. Make use of reference counting.
      return text;
    }
    // Return the remaining part of `text`.
    return text.substr(start);
  }

Cow_String std_string_trim_right(const Cow_String &text, const Cow_String &reject)
  {
    if(reject.empty()) {
      // There is no character to strip. Make use of reference counting.
      return text;
    }
    auto end = text.find_last_not_of(reject);
    if(end == Cow_String::npos) {
      // There is no character to keep. Return an empty string.
      return rocket::sref("");
    }
    if(end == text.size() - 1) {
      // There is no character to strip. Make use of reference counting.
      return text;
    }
    // Return the remaining part of `text`.
    return text.substr(0, end + 1);
  }

D_object create_bindings_string()
  {
    D_object ro;
    //===================================================================
    // `std.string.compare()`
    //===================================================================
    ro.try_emplace(rocket::sref("compare"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.compare(text_one, text_two, [length])`"
                     "\n  * Performs lexicographical comparison on two byte strings. If    "
                     "\n    `length` is set to an `integer`, no more than this number of   "
                     "\n    bytes are compared. This function behaves like the `strncmp()` "
                     "\n    function in C, except that null characters do not terminate    "
                     "\n    strings.                                                       "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.compare"), args);
            // Parse arguments.
            Cow_String text_one;
            Cow_String text_two;
            D_integer length = INT64_MAX;
            if(reader.start().req(text_one).req(text_two).opt(length).finish()) {
              // Call the binding function.
              D_integer cmp = std_string_compare(text_one, text_two, static_cast<std::size_t>(rocket::clamp(length, 0, PTRDIFF_MAX)));
              // Forward the result.
              Reference_Root::S_temporary ref_c = { cmp };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.string.reverse()`
    //===================================================================
    ro.try_emplace(rocket::sref("reverse"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.reverse(text)`"
                     "\n  * Reverses a byte string.                                        "
                     "\n  * Returns the reversed byte string.                              "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.reverse"), args);
            // Parse arguments.
            Cow_String text;
            if(reader.start().req(text).finish()) {
              // Call the binding function.
              D_string rev = std_string_reverse(text);
              // Forward the result.
              Reference_Root::S_temporary ref_c = { rocket::move(rev) };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.string.trim()`
    //===================================================================
    ro.try_emplace(rocket::sref("trim"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.trim(text, [reject])`"
                     "\n  * Removes the longest prefix and suffix consisting solely bytes  "
                     "\n    from `reject`. If `reject` is empty, no character is removed.  "
                     "\n    If `reject` is not specified, spaces and tabs are removed.     "
                     "\n  * Returns the trimmed string.                                    "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.trim"), args);
            // Parse arguments.
            Cow_String text;
            Cow_String reject = rocket::sref(" \t");
            if(reader.start().req(text).opt(reject).finish()) {
              // Call the binding function.
              D_string res = std_string_trim(text, reject);
              // Forward the result.
              Reference_Root::S_temporary ref_c = { rocket::move(res) };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.string.trim_left()`
    //===================================================================
    ro.try_emplace(rocket::sref("trim_left"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.trim_left(text, [reject])`"
                     "\n  * Removes the longest prefix consisting solely bytes from        "
                     "\n    `reject`. If `reject` is empty, no character is removed. If    "
                     "\n    `reject` is not specified, spaces and tabs are removed.        "
                     "\n  * Returns the trimmed string.                                    "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.trim_left"), args);
            // Parse arguments.
            Cow_String text;
            Cow_String reject = rocket::sref(" \t");
            if(reader.start().req(text).opt(reject).finish()) {
              // Call the binding function.
              D_string res = std_string_trim_left(text, reject);
              // Forward the result.
              Reference_Root::S_temporary ref_c = { rocket::move(res) };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.string.trim_right()`
    //===================================================================
    ro.try_emplace(rocket::sref("trim_right"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.trim_right(text, [reject])`"
                     "\n  * Removes the longest suffix consisting solely bytes from        "
                     "\n    `reject`. If `reject` is empty, no character is removed. If    "
                     "\n    `reject` is not specified, spaces and tabs are removed.        "
                     "\n  * Returns the trimmed string.                                    "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.trim_right"), args);
            // Parse arguments.
            Cow_String text;
            Cow_String reject = rocket::sref(" \t");
            if(reader.start().req(text).opt(reject).finish()) {
              // Call the binding function.
              D_string res = std_string_trim_right(text, reject);
              // Forward the result.
              Reference_Root::S_temporary ref_c = { rocket::move(res) };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // End of `std.string`
    //===================================================================
    return ro;
  }

}  // namespace Asteria
