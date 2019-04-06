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

    template<typename IteratorT> Opt<IteratorT> do_find_opt(IteratorT begin, IteratorT end, const Value& target)
      {
        for(auto it = rocket::move(begin); it != end; ++it) {
          if(it->compare(target) == Value::compare_equal) {
            return rocket::move(it);
          }
        }
        return rocket::nullopt;
      }

    }

Opt<D_integer> std_array_find(const D_array& data, const Value& target)
  {
    auto qit = do_find_opt(data.begin(), data.end(), target);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - data.begin();
  }

Opt<D_integer> std_array_rfind(const D_array& data, const Value& target)
  {
    auto qit = do_find_opt(data.rbegin(), data.rend(), target);
    if(!qit) {
      return rocket::nullopt;
    }
    return data.rend() - *qit - 1;
  }

    namespace {

    inline void do_push_argument(Cow_Vector<Reference>& args, const Value& value)
      {
        Reference_Root::S_temporary xref = { value };
        args.emplace_back(rocket::move(xref));
      }

    template<typename IteratorT> Opt<IteratorT> do_find_if_opt(const Global_Context& global, IteratorT begin, IteratorT end, const D_function& predictor, bool match)
      {
        for(auto it = rocket::move(begin); it != end; ++it) {
          // Set up arguments for the user-defined predictor.
          Cow_Vector<Reference> args;
          do_push_argument(args, *it);
          // Call it.
          Reference self;
          predictor.get().invoke(self, global, rocket::move(args));
          if(self.read().test() == match) {
            return rocket::move(it);
          }
        }
        return rocket::nullopt;
      }

    }

Opt<D_integer> std_array_find_if(const Global_Context& global, const D_array& data, const D_function& predictor)
  {
    auto qit = do_find_if_opt(global, data.begin(), data.end(), predictor, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - data.begin();
  }

Opt<D_integer> std_array_find_if_not(const Global_Context& global, const D_array& data, const D_function& predictor)
  {
    auto qit = do_find_if_opt(global, data.begin(), data.end(), predictor, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - data.begin();
  }

Opt<D_integer> std_array_rfind_if(const Global_Context& global, const D_array& data, const D_function& predictor)
  {
    auto qit = do_find_if_opt(global, data.rbegin(), data.rend(), predictor, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return data.rend() - *qit - 1;
  }

Opt<D_integer> std_array_rfind_if_not(const Global_Context& global, const D_array& data, const D_function& predictor)
  {
    auto qit = do_find_if_opt(global, data.rbegin(), data.rend(), predictor, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return data.rend() - *qit - 1;
  }

    namespace {

    Value::Compare do_compare(const Global_Context& global, const Opt<D_function>& comparator, const Value& lhs, const Value& rhs)
      {
        if(!comparator) {
          // Perform the builtin 3-way comparison.
          return lhs.compare(rhs);
        }
        // Set up arguments for the user-defined comparator.
        Cow_Vector<Reference> args;
        do_push_argument(args, lhs);
        do_push_argument(args, rhs);
        // Call it.
        Reference self;
        comparator->get().invoke(self, global, rocket::move(args));
        auto qint = self.read().opt<D_integer>();
        if(qint) {
          // Translate the result.
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
    for(;;) {
      auto next = cur + 1;
      if(next == data.end()) {
        break;
      }
      // Compare the two elements.
      auto cmp = do_compare(global, comparator, *cur, *next);
      if(rocket::is_any_of(cmp, { Value::compare_greater, Value::compare_unordered })) {
        return false;
      }
      cur = next;
    }
    return true;
  }

    namespace {

    template<typename IteratorT> std::pair<IteratorT, bool> do_bsearch(const Global_Context& global, IteratorT begin, IteratorT end,
                                                                       const Opt<D_function>& comparator, const Value& target)
      {
        auto bpos = rocket::move(begin);
        auto epos = rocket::move(end);
        bool found = false;
        for(;;) {
          auto dist = epos - bpos;
          if(dist <= 0) {
            break;
          }
          auto mpos = bpos + dist / 2;
          // Compare `target` with the element in the middle.
          auto cmp = do_compare(global, comparator, target, *mpos);
          if(cmp == Value::compare_unordered) {
            ASTERIA_THROW_RUNTIME_ERROR("The elements `", target, "` and `", *mpos, "` are unordered.");
          }
          if(cmp == Value::compare_equal) {
            bpos = mpos;
            found = true;
            break;
          }
          if(cmp == Value::compare_less) {
            epos = mpos;
            continue;
          }
          bpos = mpos + 1;
        }
        return std::make_pair(rocket::move(bpos), found);
      }

    template<typename IteratorT, typename PredT> IteratorT do_bound(const Global_Context& global, IteratorT begin, IteratorT end,
                                                                    const Opt<D_function>& comparator, const Value& target, PredT&& pred)
      {
        auto bpos = rocket::move(begin);
        auto epos = rocket::move(end);
        for(;;) {
          auto dist = epos - bpos;
          if(dist <= 0) {
            break;
          }
          auto mpos = bpos + dist / 2;
          // Compare `target` with the element in the middle.
          auto cmp = do_compare(global, comparator, target, *mpos);
          if(cmp == Value::compare_unordered) {
            ASTERIA_THROW_RUNTIME_ERROR("The elements `", target, "` and `", *mpos, "` are unordered.");
          }
          if(rocket::forward<PredT>(pred)(cmp)) {
            epos = mpos;
            continue;
          }
          bpos = mpos + 1;
        }
        return rocket::move(bpos);
      }

    }

Opt<D_integer> std_array_binary_search(const Global_Context& global, const D_array& data, const Value& target, const Opt<D_function>& comparator)
  {
    auto pair = do_bsearch(global, data.begin(), data.end(), comparator, target);
    if(!pair.second) {
      return rocket::nullopt;
    }
    return pair.first - data.begin();
  }

D_integer std_array_lower_bound(const Global_Context& global, const D_array& data, const Value& target, const Opt<D_function>& comparator)
  {
    auto lpos = do_bound(global, data.begin(), data.end(), comparator, target, [](Value::Compare cmp) { return cmp != Value::compare_greater;  });
    return lpos - data.begin();
  }

D_integer std_array_upper_bound(const Global_Context& global, const D_array& data, const Value& target, const Opt<D_function>& comparator)
  {
    auto upos = do_bound(global, data.begin(), data.end(), comparator, target, [](Value::Compare cmp) { return cmp == Value::compare_less;  });
    return upos - data.begin();
  }

std::pair<D_integer, D_integer> std_array_equal_range(const Global_Context& global, const D_array& data, const Value& target, const Opt<D_function>& comparator)
  {
    auto pair = do_bsearch(global, data.begin(), data.end(), comparator, target);
    auto lpos = do_bound(global, data.begin(), pair.first, comparator, target, [](Value::Compare cmp) { return cmp != Value::compare_greater;  });
    auto upos = do_bound(global, pair.first, data.end(), comparator, target, [](Value::Compare cmp) { return cmp == Value::compare_less;  });
    return std::make_pair(lpos - data.begin(), upos - data.begin());
  }

D_array std_array_sort(const Global_Context& global, const D_array& data, const Opt<D_function>& comparator)
  {
    return data;
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
    // `std.array.find(data, target)`
    //===================================================================
    result.insert_or_assign(rocket::sref("find"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.find(data, target)`\n"
                     "  * Finds the first element that is equal to `target`.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.find"), args);
            // Parse arguments.
            D_array data;
            Value target;
            if(reader.start().g(data).g(target).finish()) {
              // Call the binding function.
              auto qindex = std_array_find(data, target);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.rfind(data, target)`
    //===================================================================
    result.insert_or_assign(rocket::sref("rfind"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.rfind(data, target)`\n"
                     "  * Finds the last element that is equal to `target`.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.rfind"), args);
            // Parse arguments.
            D_array data;
            Value target;
            if(reader.start().g(data).g(target).finish()) {
              // Call the binding function.
              auto qindex = std_array_rfind(data, target);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.find_if(data, predictor)`
    //===================================================================
    result.insert_or_assign(rocket::sref("find_if"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.find_if(data, predictor)`\n"
                     "  * Finds the first element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically true.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.find_if"), args);
            // Parse arguments.
            D_array data;
            Opt<D_function> predictor;
            if(reader.start().g(data).g(predictor).finish()) {
              // Hmm... can `D_function` be made default-constructible?
              if(!predictor) {
                ASTERIA_THROW_RUNTIME_ERROR("`null` cannot be used as a predictor.");
              }
              // Call the binding function.
              auto qindex = std_array_find_if(global, data, *predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.find_if_not(data, predictor)`
    //===================================================================
    result.insert_or_assign(rocket::sref("find_if_not"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.find_if_not(data, predictor)`\n"
                     "  * Finds the first element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically false.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.find_if_not"), args);
            // Parse arguments.
            D_array data;
            Opt<D_function> predictor;
            if(reader.start().g(data).g(predictor).finish()) {
              // Hmm... can `D_function` be made default-constructible?
              if(!predictor) {
                ASTERIA_THROW_RUNTIME_ERROR("`null` cannot be used as a predictor.");
              }
              // Call the binding function.
              auto qindex = std_array_find_if_not(global, data, *predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.rfind_if(data, predictor)`
    //===================================================================
    result.insert_or_assign(rocket::sref("rfind_if"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.rfind_if(data, predictor)`\n"
                     "  * Finds the last elements, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically true.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.rfind_if"), args);
            // Parse arguments.
            D_array data;
            Opt<D_function> predictor;
            if(reader.start().g(data).g(predictor).finish()) {
              // Hmm... can `D_function` be made default-constructible?
              if(!predictor) {
                ASTERIA_THROW_RUNTIME_ERROR("`null` cannot be used as a predictor.");
              }
              // Call the binding function.
              auto qindex = std_array_rfind_if(global, data, *predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.rfind_if_not(data, predictor)`
    //===================================================================
    result.insert_or_assign(rocket::sref("rfind_if_not"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.rfind_if_not(data, predictor)`\n"
                     "  * Finds the last elements, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically false.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.rfind_if_not"), args);
            // Parse arguments.
            D_array data;
            Opt<D_function> predictor;
            if(reader.start().g(data).g(predictor).finish()) {
              // Hmm... can `D_function` be made default-constructible?
              if(!predictor) {
                ASTERIA_THROW_RUNTIME_ERROR("`null` cannot be used as a predictor.");
              }
              // Call the binding function.
              auto qindex = std_array_rfind_if_not(global, data, *predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
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
    // `std.array.binary_search()`
    //===================================================================
    result.insert_or_assign(rocket::sref("binary_search"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.binary_search(data, target, [comparator])`\n"
                     "  * Finds the first element in `data` that is equal to `target`.\n"
                     "    The principle of user-defined `comparator`s is the same as the\n"
                     "    `is_sorted()` function. As a consequence, the function call\n"
                     "    `is_sorted(data, comparator)` shall yield `true` prior to this\n"
                     "    call, otherwise the effect is undefined.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"
                     "  * Throws an exception if `data` has not been sorted properly. Be\n"
                     "    advised that in this case there is no guarantee whether an\n"
                     "    exception will be thrown or not.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.binary_search"), args);
            // Parse arguments.
            D_array data;
            Value target;
            Opt<D_function> comparator;
            if(reader.start().g(data).g(target).g(comparator).finish()) {
              // Call the binding function.
              auto qindex = std_array_binary_search(global, data, target, comparator);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.lower_bound()`
    //===================================================================
    result.insert_or_assign(rocket::sref("lower_bound"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.lower_bound(data, target, [comparator])`\n"
                     "  * Finds the first element in `data` that is greater than or equal\n"
                     "    to `target` and precedes all elements that are less than\n"
                     "    `target` if any. The principle of user-defined `comparator`s is\n"
                     "    the same as the `is_sorted()` function. As a consequence, the\n"
                     "    function call `is_sorted(data, comparator)` shall yield `true`\n"
                     "    prior to this call, otherwise the effect is undefined.\n"
                     "  * Returns the subscript of such an element as an `integer`. This\n"
                     "    function returns `lengthof(data)` if all elements are less than\n"
                     "    `target`.\n"
                     "  * Throws an exception if `data` has not been sorted properly. Be\n"
                     "    advised that in this case there is no guarantee whether an\n"
                     "    exception will be thrown or not.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.lower_bound"), args);
            // Parse arguments.
            D_array data;
            Value target;
            Opt<D_function> comparator;
            if(reader.start().g(data).g(target).g(comparator).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_lower_bound(global, data, target, comparator) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.upper_bound()`
    //===================================================================
    result.insert_or_assign(rocket::sref("upper_bound"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.upper_bound(data, target, [comparator])`\n"
                     "  * Finds the first element in `data` that is greater than `target`\n"
                     "    and precedes all elements that are less than or equal to\n"
                     "    `target` if any. The principle of user-defined `comparator`s is\n"
                     "    the same as the `is_sorted()` function. As a consequence, the\n"
                     "    function call `is_sorted(data, comparator)` shall yield `true`\n"
                     "    prior to this call, otherwise the effect is undefined.\n"
                     "  * Returns the subscript of such an element as an `integer`. This\n"
                     "    function returns `lengthof(data)` if all elements are less than\n"
                     "    or equal to `target`.\n"
                     "  * Throws an exception if `data` has not been sorted properly. Be\n"
                     "    advised that in this case there is no guarantee whether an\n"
                     "    exception will be thrown or not.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.upper_bound"), args);
            // Parse arguments.
            D_array data;
            Value target;
            Opt<D_function> comparator;
            if(reader.start().g(data).g(target).g(comparator).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_upper_bound(global, data, target, comparator) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.equal_range()`
    //===================================================================
    result.insert_or_assign(rocket::sref("equal_range"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.equal_range(data, target, [comparator])`\n"
                     "  * Gets the range of elements equivalent to `target` in `data` as\n"
                     "    a single function call. This function is equivalent to calling\n"
                     "    `lower_bound(data, target, comparator)` and\n"
                     "    `upper_bound(data, target, comparator)` respectively then\n"
                     "    storing both results in an `array`.\n"
                     "  * Returns an `array` of two `integer`s, the first of which\n"
                     "    specifies the lower bound and the other specifies the upper\n"
                     "    bound.\n"
                     "  * Throws an exception if `data` has not been sorted properly. Be\n"
                     "    advised that in this case there is no guarantee whether an\n"
                     "    exception will be thrown or not.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.lower_bound"), args);
            // Parse arguments.
            D_array data;
            Value target;
            Opt<D_function> comparator;
            if(reader.start().g(data).g(target).g(comparator).finish()) {
              // Call the binding function.
              auto pair = std_array_equal_range(global, data, target, comparator);
              D_array res;
              res.reserve(2);
              res.emplace_back(rocket::move(pair.first));
              res.emplace_back(rocket::move(pair.second));
              Reference_Root::S_temporary xref = { rocket::move(res) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.sort()`
    //===================================================================
    result.insert_or_assign(rocket::sref("sort"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.sort(data, [comparator])`\n"
                     "  * Sorts elements in `data` in ascending order. The principle of\n"
                     "    user-defined `comparator`s is the same as the `is_sorted()`\n"
                     "    function. The algorithm shall finish in `O(n log n)` time where\n"
                     "    `n` is the number of elements in `data`, and shall be stable.\n"
                     "    This function returns a new `array` without modifying `data`.\n"
                     "  * Returns the sorted `array`.\n"
                     "  * Throws an exception if any elements are unordered. Be advised\n"
                     "    that in this case there is no guarantee whether an exception\n"
                     "    will be thrown or not.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.sort"), args);
            // Parse arguments.
            D_array data;
            Opt<D_function> comparator;
            if(reader.start().g(data).g(comparator).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_sort(global, data, comparator) };
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
