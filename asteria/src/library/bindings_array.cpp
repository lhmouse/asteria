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

    namespace {

    inline void do_push_argument(Cow_Vector<Reference>& args, const Value& value)
      {
        Reference_Root::S_temporary xref = { value };
        args.emplace_back(rocket::move(xref));
      }

    Value::Compare do_compare(const Global_Context& global, const Opt<D_function>& comparator, Cow_Vector<Reference>&& args, const Value& lhs, const Value& rhs)
      {
        if(!comparator) {
          // Perform the builtin 3-way comparison.
          return lhs.compare(rhs);
        }
        // Set up arguments for the user-defined comparator.
        args.clear();
        do_push_argument(args, lhs);
        do_push_argument(args, rhs);
        // Call it.
        Reference self;
        comparator->get().invoke(self, global, rocket::move(args));
        // Translate the result.
        auto qint = self.read().opt<D_integer>();
        if(qint) {
          if(*qint < 0) {
            return Value::compare_less;
          }
          if(*qint > 0) {
            return Value::compare_greater;
          }
          return Value::compare_equal;
        }
        // If the result isn't an `integer`, consider `lhs` and `rhs` unordered.
        return Value::compare_unordered;
      }

    }

D_boolean std_array_is_sorted(const Global_Context& global, const D_array& data, const Opt<D_function>& comparator)
  {
    auto cur = data.begin();
    if(cur == data.end()) {
      // If `data` contains no element, it is considered sorted.
      return true;
    }
    // This can be reused.
    Cow_Vector<Reference> args;
    for(;;) {
      auto next = cur + 1;
      if(next == data.end()) {
        break;
      }
      // Compare the two elements.
      auto cmp = do_compare(global, comparator, rocket::move(args), *cur, *next);
      if(rocket::is_any_of(cmp, { Value::compare_greater, Value::compare_unordered })) {
        return false;
      }
      cur = next;
    }
    return true;
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
    // `std.array.is_sorted()`
    //===================================================================
    result.insert_or_assign(rocket::sref("is_sorted"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.is_sorted(data, [comparator])`\n"
                     "  * Checks whether `data` is sorted. That is, there is no pair of\n"
                     "    adjacent elements in `data` such that the first one is greater\n"
                     "    than or unordered with the second one. Elements are compared\n"
                     "    using `comparator`, which shall be a binary `function` that\n"
                     "    returns a negative `integer` if the first argument is less than\n"
                     "    the second one, a positive `integer` if the first argument is\n"
                     "    greater than the second one, or `0` if the arguments are equal;\n"
                     "    other values indicate that the arguments are unordered. If no\n"
                     "    `comparator` is provided, the builtin 3-way comparison operator\n"
                     "    is used. An `array` that contains no elements is considered to\n"
                     "    have been sorted.\n"
                     "  * Returns `true` if `data` is sorted or empty; otherwise `false`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.is_sorted"), args);
            // Parse arguments.
            D_array data;
            Opt<D_function> comparator;
            if(reader.start().g(data).g(comparator).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_is_sorted(global, data, comparator) };
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
