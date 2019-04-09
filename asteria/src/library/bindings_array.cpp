// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_array.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/global_context.hpp"
#include "../utilities.hpp"
#include <random>
#ifdef _WIN32
#  include <windows.h>
#else
#  include <time.h>
#endif

namespace Asteria {

    namespace {

    std::pair<D_array::const_iterator, D_array::const_iterator> do_slice(const D_array& text, D_array::const_iterator tbegin, const Opt<D_integer>& length)
      {
        if(!length || (*length >= text.end() - tbegin)) {
          // Get the subrange from `tbegin` to the end.
          return std::make_pair(tbegin, text.end());
        }
        if(*length <= 0) {
          // Return an empty range.
          return std::make_pair(tbegin, tbegin);
        }
        // Don't go past the end.
        return std::make_pair(tbegin, tbegin + static_cast<std::ptrdiff_t>(*length));
      }

    std::pair<D_array::const_iterator, D_array::const_iterator> do_slice(const D_array& text, const D_integer& from, const Opt<D_integer>& length)
      {
        auto slen = static_cast<std::int64_t>(text.size());
        if(from >= 0) {
          // Behave like `std::string::substr()` except that no exception is thrown when `from` is greater than `text.size()`.
          if(from >= slen) {
            return std::make_pair(text.end(), text.end());
          }
          return do_slice(text, text.begin() + static_cast<std::ptrdiff_t>(from), length);
        }
        // Wrap `from` from the end. Notice that `from + slen` will not overflow when `from` is negative and `slen` is not.
        auto rfrom = from + slen;
        if(rfrom >= 0) {
          // Get a subrange from the wrapped index.
          return do_slice(text, text.begin() + static_cast<std::ptrdiff_t>(rfrom), length);
        }
        // Get a subrange from the beginning of `text`, if the wrapped index is before the first byte.
        if(!length) {
          // Get the subrange from the beginning to the end.
          return std::make_pair(text.begin(), text.end());
        }
        if(*length <= 0) {
          // Return an empty range.
          return std::make_pair(text.begin(), text.begin());
        }
        // Get a subrange excluding the part before the beginning. Notice that `rfrom + *length` will not overflow when `rfrom` is negative and `*length` is not.
        return do_slice(text, text.begin(), rfrom + *length);
      }

    }

D_array std_array_slice(const D_array& data, const D_integer& from, const Opt<D_integer>& length)
  {
    auto range = do_slice(data, from, length);
    if((range.first == data.begin()) && (range.second == data.end())) {
      // Use reference counting as our advantage.
      return data;
    }
    return D_array(range.first, range.second);
  }

D_array std_array_replace_slice(const D_array& data, const D_integer& from, const D_array& replacement)
  {
    auto range = do_slice(data, from, rocket::nullopt);
    // Append segments.
    D_array res;
    res.reserve(data.size() - static_cast<std::size_t>(range.second - range.first) + replacement.size());
    res.append(data.begin(), range.first);
    res.append(replacement.begin(), replacement.end());
    res.append(range.second, data.end());
    return res;
  }

D_array std_array_replace_slice(const D_array& data, const D_integer& from, const Opt<D_integer>& length, const D_array& replacement)
  {
    auto range = do_slice(data, from, length);
    // Append segments.
    D_array res;
    res.reserve(data.size() - static_cast<std::size_t>(range.second - range.first) + replacement.size());
    res.append(data.begin(), range.first);
    res.append(replacement.begin(), replacement.end());
    res.append(range.second, data.end());
    return res;
  }

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

Opt<D_integer> std_array_find(const D_array& data, const D_integer& from, const Value& target)
  {
    auto range = do_slice(data, from, rocket::nullopt);
    auto qit = do_find_opt(range.first, range.second, target);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - data.begin();
  }

Opt<D_integer> std_array_find(const D_array& data, const D_integer& from, const Opt<D_integer>& length, const Value& target)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_opt(range.first, range.second, target);
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

Opt<D_integer> std_array_rfind(const D_array& data, const D_integer& from, const Value& target)
  {
    auto range = do_slice(data, from, rocket::nullopt);
    auto qit = do_find_opt(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), target);
    if(!qit) {
      return rocket::nullopt;
    }
    return data.rend() - *qit - 1;
  }

Opt<D_integer> std_array_rfind(const D_array& data, const D_integer& from, const Opt<D_integer>& length, const Value& target)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_opt(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), target);
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

Opt<D_integer> std_array_find_if(const Global_Context& global, const D_array& data, const D_integer& from, const D_function& predictor)
  {
    auto range = do_slice(data, from, rocket::nullopt);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - data.begin();
  }

Opt<D_integer> std_array_find_if(const Global_Context& global, const D_array& data, const D_integer& from, const Opt<D_integer>& length, const D_function& predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
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

Opt<D_integer> std_array_find_if_not(const Global_Context& global, const D_array& data, const D_integer& from, const D_function& predictor)
  {
    auto range = do_slice(data, from, rocket::nullopt);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - data.begin();
  }

Opt<D_integer> std_array_find_if_not(const Global_Context& global, const D_array& data, const D_integer& from, const Opt<D_integer>& length, const D_function& predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
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

Opt<D_integer> std_array_rfind_if(const Global_Context& global, const D_array& data, const D_integer& from, const D_function& predictor)
  {
    auto range = do_slice(data, from, rocket::nullopt);
    auto qit = do_find_if_opt(global, std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), predictor, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return data.rend() - *qit - 1;
  }

Opt<D_integer> std_array_rfind_if(const Global_Context& global, const D_array& data, const D_integer& from, const Opt<D_integer>& length, const D_function& predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), predictor, true);
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

Opt<D_integer> std_array_rfind_if_not(const Global_Context& global, const D_array& data, const D_integer& from, const D_function& predictor)
  {
    auto range = do_slice(data, from, rocket::nullopt);
    auto qit = do_find_if_opt(global, std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), predictor, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return data.rend() - *qit - 1;
  }

Opt<D_integer> std_array_rfind_if_not(const Global_Context& global, const D_array& data, const D_integer& from, const Opt<D_integer>& length, const D_function& predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), predictor, false);
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
    D_array res = data;
    if(res.size() <= 1) {
      // Use reference counting as our advantage.
      return res;
    }
    // Define the temporary storage for Merge Sort.
    D_array temp;
    temp.resize(res.size());
    for(std::size_t bsize = 1; bsize < res.size(); bsize *= 2) {
      // Merge adjacent blocks of `bsize` elements.
      std::size_t toff = 0;
      for(;;) {
        // Define a function to merge one element.
        auto transfer_at = [&](std::size_t roff)
          {
            temp.mut(toff++) = rocket::move(res.mut(roff));
          };
        // Define range information for blocks.
        struct Block
          {
            std::size_t off;
            std::size_t end;
          }
        r1, r2;
        // Get the range of the first block to merge.
        r1.off = toff;
        r1.end = toff + bsize;
        // Stop if there are no more blocks.
        if(res.size() <= r1.end) {
          // Copy all remaining elements.
          rocket::ranged_for(r1.off, res.size(), transfer_at);
          break;
        }
        // Get the range of the second block to merge.
        r2.off = r1.end;
        r2.end = rocket::min(r1.end + bsize, res.size());
        // Merge elements one by one, until either block has been exhausted.
  z:
        auto cmp = do_compare(global, comparator, res[r1.off], res[r2.off]);
        if(cmp == Value::compare_unordered) {
          ASTERIA_THROW_RUNTIME_ERROR("The elements `", res[r1.off], "` and `", res[r2.off], "` are unordered.");
        }
        // For Merge Sort to be stable, the two elements will only be swapped if the first one is greater than the second one.
        auto refs = (cmp != Value::compare_greater) ? std::tie(r1, r2) : std::tie(r2, r1);
        auto& rf = std::get<0>(refs);
        // Move the element from `rf`.
        transfer_at(rf.off++);
        if(rf.off != rf.end) {
          goto z;
        }
        // Move all elements from the other block.
        auto& rk = std::get<1>(refs);
        rocket::ranged_do_while(rk.off, rk.end, transfer_at);
      }
      // Accept all merged blocks.
      res.swap(temp);
    }
    return res;
  }

D_array std_array_generate(const Global_Context& global, const D_function& generator, const D_integer& length)
  {
    D_array res;
    res.reserve(static_cast<std::size_t>(length));
    for(std::int64_t i = 0; i < length; ++i) {
      // Set up arguments for the user-defined generator.
      Cow_Vector<Reference> args;
      do_push_argument(args, D_integer(i));
      do_push_argument(args, res.empty() ? Value::get_null() : res.back());
      // Call it.
      Reference self;
      generator.get().invoke(self, global, rocket::move(args));
      res.emplace_back(self.read());
    }
    return res;
  }

D_array std_array_shuffle(const D_array& data, const Opt<D_integer>& seed)
  {
    D_array res = data;
    if(res.size() <= 1) {
      // Use reference counting as our advantage.
      return res;
    }
    // Initialize the random generator.
    std::int64_t rseed;
    if(seed) {
      // Use the user-provided seed.
      rseed = *seed;
    } else {
      // Get a seed from the system.
#ifdef _WIN32
      ::LARGE_INTEGER li;
      ::QueryPerformanceCounter(&li);
      rseed = li.QuadPart;
#else
      ::timespec ts;
      ::clock_gettime(CLOCK_MONOTONIC, &ts);
      rseed = ts.tv_nsec;
#endif
    }
    // Create a linear congruential generator with `rseed`.
    // The template arguments are the same as glibc's `rand48_r()` function.
    //   https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
    std::linear_congruential_engine<std::uint64_t, 0x5DEECE66D, 0xB, 0x1000000000000> prng(static_cast<std::uint64_t>(rseed));
    // Shuffle elements.
    for(auto bpos = res.mut_begin(); bpos != res.end(); ++bpos) {
      // N.B. Conversion from an unsigned type to a floating-point type would result in performance penalty.
      // ratio <= [0.0, 1.0)
      auto ratio = static_cast<double>(static_cast<std::int64_t>(prng())) / 0x1p48;
      // offset <= [0, res.size())
      auto offset = static_cast<std::ptrdiff_t>(ratio * static_cast<double>(res.ssize()));
      // Swap `*bpos` with the element at `offset`.
      std::iter_swap(bpos, res.mut_begin() + offset);
    }
    return res;
  }

void create_bindings_array(D_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.array.slice()`
    //===================================================================
    result.insert_or_assign(rocket::sref("slice"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.slice(data, from, [length])`\n"
                     "  * Copies a subrange of `data` to create a new `array`. Elements\n"
                     "    are copied from `from` if it is non-negative, and from\n"
                     "    `lengthof(data) + from` otherwise. If `length` is set to an\n"
                     "    `integer`, no more than this number of elements will be copied.\n"
                     "    If it is absent, all elements from `from` to the end of `data`\n"
                     "    will be copied. If `from` is outside `data`, an empty `array`\n"
                     "    is returned.\n"
                     "  * Returns the specified subarray of `data`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.slice"), args);
            // Parse arguments.
            D_array data;
            D_integer from;
            Opt<D_integer> length;
            if(reader.start().g(data).g(from).g(length).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_slice(data, from, length) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.replace_slice()`
    //===================================================================
    result.insert_or_assign(rocket::sref("replace_slice"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.replace_slice(data, from, replacement)`\n"
                     "  * Replaces all elements from `from` to the end of `data` with\n"
                     "    `replacement` and returns the new `array`. If `from` is\n"
                     "    negative, it specifies an offset from the end of `data`. This\n"
                     "    function returns a new `array` without modifying `data`.\n"
                     "  * Returns a `array` with the subrange replaced.\n"
                     "`std.array.replace_slice(data, from, [length], replacement)`\n"
                     "  * Replaces a subrange of `data` with `replacement` to create a\n"
                     "    new `array`. `from` specifies the start of the subrange to\n"
                     "    replace. If `from` is negative, it specifies an offset from the\n"
                     "    end of `data`. `length` specifies the maximum number of\n"
                     "    elements to replace. If it is set to `null`, this function is\n"
                     "    equivalent to `replace_slice(data, from, replacement)`. This\n"
                     "    function returns a new `array` without modifying `data`.\n"
                     "  * Returns a `array` with the subrange replaced.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.replace"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_array data;
            D_integer from;
            D_array replacement;
            if(reader.start().g(data).g(from).save_state(state).g(replacement).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_replace_slice(data, from, replacement) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(replacement).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_replace_slice(data, from, length, replacement) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
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
    // `std.array.find()`
    //===================================================================
    result.insert_or_assign(rocket::sref("find"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.find(data, target)`\n"
                     "  * Searches `data` for the first occurrence of `target`.\n"
                     "  * Returns the subscript of the first match of `target` in `data`\n"
                     "    if one is found, which is always non-negative; otherwise\n"
                     "    `null`.\n"
                     "`std.array.find(data, from, target)`\n"
                     "  * Searches `data` for the first occurrence of `target`. The\n"
                     "    search operation is performed on the same subrange that would\n"
                     "    be returned by `slice(data, from)`.\n"
                     "  * Returns the subscript of the first match of `target` in `data`\n"
                     "    if one is found, which is always non-negative; otherwise\n"
                     "    `null`.\n"
                     "`std.array.find(data, from, [length], target)`\n"
                     "  * Searches `data` for the first occurrence of `target`. The\n"
                     "    search operation is performed on the same subrange that would\n"
                     "    be returned by `slice(data, from, length)`.\n"
                     "  * Returns the subscript of the first match of `target` in `data`\n"
                     "    if one is found, which is always non-negative; otherwise\n"
                     "    `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.find"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_array data;
            Value target;
            if(reader.start().g(data).save_state(state).g(target).finish()) {
              // Call the binding function.
              auto qindex = std_array_find(data, target);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            D_integer from;
            if(reader.load_state(state).g(from).save_state(state).g(target).finish()) {
              // Call the binding function.
              auto qindex = std_array_find(data, from, target);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(target).finish()) {
              // Call the binding function.
              auto qindex = std_array_find(data, from, length, target);
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
    // `std.array.rfind()`
    //===================================================================
    result.insert_or_assign(rocket::sref("rfind"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.rfind(data, target)`\n"
                     "  * Searches `data` for the last occurrence of `target`.\n"
                     "  * Returns the subscript of the last match of `target` in `data`\n"
                     "    if one is found, which is always non-negative; otherwise\n"
                     "    `null`.\n"
                     "`std.array.rfind(data, from, target)`\n"
                     "  * Searches `data` for the last occurrence of `target`. The search\n"
                     "    operation is performed on the same subrange that would be\n"
                     "    returned by `slice(data, from)`.\n"
                     "  * Returns the subscript of the last match of `target` in `data`\n"
                     "    if one is found, which is always non-negative; otherwise\n"
                     "    `null`.\n"
                     "`std.array.rfind(data, from, [length], target)`\n"
                     "  * Searches `data` for the last occurrence of `target`. The search\n"
                     "    operation is performed on the same subrange that would be\n"
                     "    returned by `slice(data, from, length)`.\n"
                     "  * Returns the subscript of the last match of `target` in `data`\n"
                     "    if one is found, which is always non-negative; otherwise\n"
                     "    `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.rfind"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_array data;
            Value target;
            if(reader.start().g(data).save_state(state).g(target).finish()) {
              // Call the binding function.
              auto qindex = std_array_rfind(data, target);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            D_integer from;
            if(reader.load_state(state).g(from).save_state(state).g(target).finish()) {
              // Call the binding function.
              auto qindex = std_array_rfind(data, from, target);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(target).finish()) {
              // Call the binding function.
              auto qindex = std_array_rfind(data, from, length, target);
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
    // `std.array.find_if()`
    //===================================================================
    result.insert_or_assign(rocket::sref("find_if"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.find_if(data, predictor)`\n"
                     "  * Finds the first element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically true.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"
                     "`std.array.find_if(data, from, predictor)`\n"
                     "  * Finds the first element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically true. The search operation is\n"
                     "    performed on the same subrange that would be returned by\n"
                     "    `slice(data, from)`.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"
                     "`std.array.find_if(data, from, [length], predictor)`\n"
                     "  * Finds the first element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically true. The search operation is\n"
                     "    performed on the same subrange that would be returned by\n"
                     "    `slice(data, from, length)`.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.find_if"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_array data;
            D_function predictor = global.uninitialized_placeholder();
            if(reader.start().g(data).save_state(state).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_find_if(global, data, predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            D_integer from;
            if(reader.load_state(state).g(from).save_state(state).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_find_if(global, data, from, predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_find_if(global, data, from, length, predictor);
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
    // `std.array.find_if_not()`
    //===================================================================
    result.insert_or_assign(rocket::sref("find_if_not"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.find_if_not(data, predictor)`\n"
                     "  * Finds the first element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically false.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"
                     "`std.array.find_if_not(data, from, predictor)`\n"
                     "  * Finds the first element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically false. The search operation is\n"
                     "    performed on the same subrange that would be returned by\n"
                     "    `slice(data, from)`.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"
                     "`std.array.find_if_not(data, from, [length], predictor)`\n"
                     "  * Finds the first element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically false. The search operation is\n"
                     "    performed on the same subrange that would be returned by\n"
                     "    `slice(data, from, length)`.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.find_if_not"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_array data;
            D_function predictor = global.uninitialized_placeholder();
            if(reader.start().g(data).save_state(state).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_find_if_not(global, data, predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            D_integer from;
            if(reader.load_state(state).g(from).save_state(state).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_find_if_not(global, data, from, predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_find_if_not(global, data, from, length, predictor);
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
    // `std.array.rfind_if()`
    //===================================================================
    result.insert_or_assign(rocket::sref("rfind_if"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.rfind_if(data, predictor)`\n"
                     "  * Finds the last element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically true.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"
                     "`std.array.rfind_if(data, from, predictor)`\n"
                     "  * Finds the last element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically true. The search operation is\n"
                     "    performed on the same subrange that would be returned by\n"
                     "    `slice(data, from)`.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"
                     "`std.array.rfind_if(data, from, [length], predictor)`\n"
                     "  * Finds the last element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically true. The search operation is\n"
                     "    performed on the same subrange that would be returned by\n"
                     "    `slice(data, from, length)`.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.rfind_if"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_array data;
            D_function predictor = global.uninitialized_placeholder();
            if(reader.start().g(data).save_state(state).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_rfind_if(global, data, predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            D_integer from;
            if(reader.load_state(state).g(from).save_state(state).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_rfind_if(global, data, from, predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_rfind_if(global, data, from, length, predictor);
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
    // `std.array.rfind_if_not()`
    //===================================================================
    result.insert_or_assign(rocket::sref("rfind_if_not"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.rfind_if_not(data, predictor)`\n"
                     "  * Finds the last element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically false.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"
                     "`std.array.rfind_if_not(data, from, predictor)`\n"
                     "  * Finds the last element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically false. The search operation is\n"
                     "    performed on the same subrange that would be returned by\n"
                     "    `slice(data, from)`.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"
                     "`std.array.rfind_if_not(data, from, [length], predictor)`\n"
                     "  * Finds the last element, namely `x`, in `data`, for which\n"
                     "    `predictor(x)` yields logically false. The search operation is\n"
                     "    performed on the same subrange that would be returned by\n"
                     "    `slice(data, from, length)`.\n"
                     "  * Returns the subscript of such an element as an `integer`, if\n"
                     "    one is found; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.rfind_if_not"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_array data;
            D_function predictor = global.uninitialized_placeholder();
            if(reader.start().g(data).save_state(state).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_rfind_if_not(global, data, predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            D_integer from;
            if(reader.load_state(state).g(from).save_state(state).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_rfind_if_not(global, data, from, predictor);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(predictor).finish()) {
              // Call the binding function.
              auto qindex = std_array_rfind_if_not(global, data, from, length, predictor);
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
    // `std.array.generate()`
    //===================================================================
    result.insert_or_assign(rocket::sref("generate"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.generate(generator, length)`\n"
                     "  * Calls `generator` repeatedly up to `length` times and returns\n"
                     "    an `array` consisting of all values returned. `generator` shall\n"
                     "    be a binary function. The first argument will be the number of\n"
                     "    elements having been generated; the second argument is the\n"
                     "    previous element generated, or `null` in the case of the first\n"
                     "    element.\n"
                     "  * Returns an `array` containing all values generated.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& global, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.generate"), args);
            // Parse arguments.
            D_function generator = global.uninitialized_placeholder();
            D_integer length;
            if(reader.start().g(generator).g(length).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_generate(global, generator, length) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.array.shuffle()`
    //===================================================================
    result.insert_or_assign(rocket::sref("shuffle"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.array.shuffle(data, [seed])`\n"
                     "  * Shuffles elements in `data` randomly. If `seed` is set to an\n"
                     "    `integer`, the internal pseudo random number generator will be\n"
                     "    initialized with it and will produce the same series of numbers\n"
                     "    for a specific `seed` value. If it is absent, an unspecified\n"
                     "    seed is generated when this function is called. This function\n"
                     "    returns a new `array` without modifying `data`.\n"
                     "  * Returns the shuffled `array`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.array.shuffle"), args);
            // Parse arguments.
            D_array data;
            Opt<D_integer> seed;
            if(reader.start().g(data).g(seed).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_array_shuffle(data, seed) };
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
