// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_array.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

namespace Asteria {

Value std_array_max_of(const D_array& data)
  {
    auto maxp = data.begin();
    if(maxp == data.end()) {
      // Return `null` if `data` is empty.
      return D_null();
    }
    for(auto cur = maxp + 1; cur != data.end(); ++cur) {
      // Compare `*maxp` with the other elements, ignoring unordered elements.
      if(maxp->compare(*cur) != Value::compare_less) {
        continue;
      }
      maxp = cur;
    }
    return *maxp;
  }

Value std_array_min_of(const D_array& data)
  {
    auto minp = data.begin();
    if(minp == data.end()) {
      // Return `null` if `data` is empty.
      return D_null();
    }
    for(auto cur = minp + 1; cur != data.end(); ++cur) {
      // Compare `*minp` with the other elements, ignoring unordered elements.
      if(minp->compare(*cur) != Value::compare_greater) {
        continue;
      }
      minp = cur;
    }
    return *minp;
  }

void create_bindings_array(D_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.array.max_of()`
    //===================================================================
    result.insert_or_assign(rocket::sref("max_of"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.max_of(data)`\n"
                     "  * Finds and returns the maximum element in `data`. Elements that\n"
                     "    are unordered with the first element are ignored.\n"
                     "  * Returns a copy of the maximum element, or `null` if `data` is\n"
                     "    empty.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.max_of"), args);
            // Parse arguments.
            D_array data;
            if(reader.start().g(data).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_max_of(data) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.min_of()`
    //===================================================================
    result.insert_or_assign(rocket::sref("min_of"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.min_of(data)`\n"
                     "  * Finds and returns the minimum element in `data`. Elements that\n"
                     "    are unordered with the first element are ignored.\n"
                     "  * Returns a copy of the minimum element, or `null` if `data` is\n"
                     "    empty.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.min_of"), args);
            // Parse arguments.
            D_array data;
            if(reader.start().g(data).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_min_of(data) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // End of `std.array`
    //===================================================================
  }

}  // namespace Asteria
