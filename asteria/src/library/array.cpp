// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "array.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../utilities.hpp"

namespace asteria {
namespace {

pair<V_array::const_iterator, V_array::const_iterator>
do_slice(const V_array& data, V_array::const_iterator tbegin, const Opt_integer& length)
  {
    if(!length || (*length >= data.end() - tbegin))
      return ::std::make_pair(tbegin, data.end());

    if(*length <= 0)
      return ::std::make_pair(tbegin, tbegin);

    // Don't go past the end.
    return ::std::make_pair(tbegin, tbegin + static_cast<ptrdiff_t>(*length));
  }

pair<V_array::const_iterator, V_array::const_iterator>
do_slice(const V_array& data, const V_integer& from, const Opt_integer& length)
  {
    auto slen = static_cast<int64_t>(data.size());
    if(from >= 0) {
      // Behave like `::std::string::substr()` except that no exception is thrown when `from` is
      // greater than `data.size()`.
      if(from >= slen)
        return ::std::make_pair(data.end(), data.end());

      // Return a subrange from `begin() + from`.
      return do_slice(data, data.begin() + static_cast<ptrdiff_t>(from), length);
    }

    // Wrap `from` from the end. Notice that `from + slen` will not overflow when `from` is negative
    // and `slen` is not.
    auto rfrom = from + slen;
    if(rfrom >= 0)
      return do_slice(data, data.begin() + static_cast<ptrdiff_t>(rfrom), length);

    // Get a subrange from the beginning of `data`, if the wrapped index is before the first byte.
    if(!length)
      return ::std::make_pair(data.begin(), data.end());

    if(*length <= 0)
      return ::std::make_pair(data.begin(), data.begin());

    // Get a subrange excluding the part before the beginning.
    // Notice that `rfrom + *length` will not overflow when `rfrom` is negative and `*length` is not.
    return do_slice(data, data.begin(), rfrom + *length);
  }

template<typename IterT>
opt<IterT>
do_find_opt(IterT begin, IterT end, const Value& target)
  {
    for(auto it = ::std::move(begin);  it != end;  ++it) {
      // Compare the value using the builtin 3-way comparison operator.
      if(it->compare(target) == compare_equal)
        return ::std::move(it);
    }
    // Fail to find an element.
    return nullopt;
  }

Reference_root::S_temporary
do_make_temporary(const Value& value)
  {
    Reference_root::S_temporary xref = { value };
    return xref;
  }

template<typename IterT>
opt<IterT>
do_find_if_opt(Global_Context& global, IterT begin, IterT end, const V_function& pred, bool match)
  {
    cow_vector<Reference> args;
    for(auto it = ::std::move(begin);  it != end;  ++it) {
      // Set up arguments for the user-defined predictor.
      args.resize(1);
      args.mut(0) = do_make_temporary(*it);
      // Call the predictor function and check the return value.
      auto self = pred.invoke(global, ::std::move(args));
      if(self.read().test() == match)
        return ::std::move(it);
    }
    // Fail to find an element.
    return nullopt;
  }

Compare
do_compare(Global_Context& global, cow_vector<Reference>& args,
           const Opt_function& kcomp, const Value& lhs, const Value& rhs)
  {
    // Use the builtin 3-way comparison operator if no comparator is provided.
    if(ROCKET_EXPECT(!kcomp))
      return lhs.compare(rhs);

    // Set up arguments for the user-defined comparator.
    args.resize(2);
    args.mut(0) = do_make_temporary(lhs);
    args.mut(1) = do_make_temporary(rhs);
    // Call the predictor function and compare the result with `0`.
    auto self = kcomp.invoke(global, ::std::move(args));
    return self.read().compare(V_integer(0));
  }

template<typename IterT>
pair<IterT, bool>
do_bsearch(Global_Context& global, cow_vector<Reference>& args, IterT begin, IterT end,
           const Opt_function& kcomp, const Value& target)
  {
    auto bpos = ::std::move(begin);
    auto epos = ::std::move(end);
    for(;;) {
      auto dist = epos - bpos;
      if(dist <= 0)
        return { ::std::move(bpos), false };

      // Compare `target` to the element in the middle.
      auto mpos = bpos + dist / 2;
      auto cmp = do_compare(global, args, kcomp, target, *mpos);
      if(cmp == compare_unordered)
        ASTERIA_THROW("Unordered elements (operands were `$1` and `$2`)", target, *mpos);

      if(cmp == compare_equal)
        return { ::std::move(mpos), true };

      if(cmp == compare_less)
        epos = mpos;
      else
        bpos = mpos + 1;
    }
  }

template<typename IterT, typename PredT>
IterT
do_bound(Global_Context& global, cow_vector<Reference>& args, IterT begin, IterT end,
         const Opt_function& kcomp, const Value& target, PredT&& pred)
  {
    auto bpos = ::std::move(begin);
    auto epos = ::std::move(end);
    for(;;) {
      auto dist = epos - bpos;
      if(dist <= 0)
        return bpos;

      // Compare `target` to the element in the middle.
      auto mpos = bpos + dist / 2;
      auto cmp = do_compare(global, args, kcomp, target, *mpos);
      if(cmp == compare_unordered)
        ASTERIA_THROW("Unordered elements (operands were `$1` and `$2`)", target, *mpos);

      if(pred(cmp))
        epos = mpos;
      else
        bpos = mpos + 1;
    }
  }

V_array::iterator&
do_merge_range(V_array::iterator& opos, Global_Context& global, cow_vector<Reference>& args,
               const Opt_function& kcomp, V_array::iterator ibegin, V_array::iterator iend, bool unique)
  {
    for(auto ipos = ibegin;  ipos != iend;  ++ipos)
      if(!unique || (do_compare(global, args, kcomp, ipos[0], opos[-1]) != compare_equal))
        *(opos++) = ::std::move(*ipos);
    return opos;
  }

V_array::iterator
do_merge_blocks(V_array& output, Global_Context& global, cow_vector<Reference>& args,
                const Opt_function& kcomp, V_array&& input, ptrdiff_t bsize, bool unique)
  {
    ROCKET_ASSERT(output.size() >= input.size());

    // Define the range information for a pair of contiguous blocks.
    V_array::iterator bpos[2];
    V_array::iterator bend[2];

    // Merge adjacent blocks of `bsize` elements.
    auto opos = output.mut_begin();
    auto ipos = input.mut_begin();
    auto iend = input.mut_end();
    while(iend - ipos > bsize) {
      // Get the range of the first block to merge.
      bpos[0] = ipos;
      ipos += bsize;
      bend[0] = ipos;

      // Get the range of the second block to merge.
      bpos[1] = ipos;
      ipos += ::rocket::min(iend - ipos, bsize);
      bend[1] = ipos;

      // Merge elements one by one, until either block has been exhausted, then store the index of it here.
      size_t bi;
      for(;;) {
        auto cmp = do_compare(global, args, kcomp, *(bpos[0]), *(bpos[1]));
        if(cmp == compare_unordered)
          ASTERIA_THROW("Unordered elements (operands were `$1` and `$2`)", *(bpos[0]), *(bpos[1]));

        // For Merge Sort to be stable, the two elements will only be swapped if the first one is greater
        // than the second one.
        bi = (cmp == compare_greater);

        // Move this element unless uniqueness is requested and it is equal to the previous output.
        bool discard = unique && (opos != output.begin())
                              && (do_compare(global, args, kcomp, *(bpos[bi]), opos[-1]) == compare_equal);
        if(!discard)
          *(opos++) = ::std::move(*(bpos[bi]));
        bpos[bi]++;

        // When uniqueness is requested, if elements from the two blocks are equal, discard the one from
        // the second block. This may exhaust the second block.
        if(unique && (cmp == compare_equal)) {
          size_t oi = bi ^ 1;
          bpos[oi]++;
          if(bpos[oi] == bend[oi]) {
            // `bi` is the index of the block that has been exhausted.
            bi = oi;
            break;
          }
        }
        if(bpos[bi] == bend[bi])
          // `bi` is the index of the block that has been exhausted.
          break;
      }
      // Move all elements from the other block.
      ROCKET_ASSERT(opos != output.begin());
      bi ^= 1;
      do_merge_range(opos, global, args, kcomp, bpos[bi], bend[bi], unique);
    }
    // Copy all remaining elements.
    ROCKET_ASSERT(opos != output.begin());
    do_merge_range(opos, global, args, kcomp, ipos, iend, unique);
    return opos;
  }

}  // namespace

V_array
std_array_slice(V_array data, V_integer from, Opt_integer length)
  {
    auto range = do_slice(data, from, length);
    if((range.first == data.begin()) && (range.second == data.end()))
      // Use reference counting as our advantage.
      return data;
    return V_array(range.first, range.second);
  }

V_array
std_array_replace_slice(V_array data, V_integer from, Opt_integer length, V_array replacement)
  {
    V_array res = data;
    auto range = do_slice(res, from, length);

    // Convert iterators to subscripts.
    auto offset = ::std::distance(res.begin(), range.first);
    auto dist = ::std::distance(range.first, range.second);

    if(dist >= replacement.ssize()) {
      // Overwrite the subrange in place.
      res.erase(::std::move(replacement.mut_begin(), replacement.mut_end(),
                            res.mut_begin() + offset),
                res.mut_begin() + offset + dist);
    }
    else {
      // Extend the range.
      res.insert(::std::move(replacement.mut_begin(), replacement.mut_begin() + dist,
                             res.mut_begin() + offset),
                 replacement.move_begin() + dist, replacement.move_end());
    }
    return res;
  }

Opt_integer
std_array_find(V_array data, V_integer from, Opt_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_opt(range.first, range.second, target);
    if(!qit)
      return nullopt;
    return *qit - data.begin();
  }

Opt_integer
std_array_find_if(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                  V_function predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
    if(!qit)
      return nullopt;
    return *qit - data.begin();
  }

Opt_integer
std_array_find_if_not(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                      V_function predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
    if(!qit)
      return nullopt;
    return *qit - data.begin();
  }

Opt_integer
std_array_rfind(V_array data, V_integer from, Opt_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second),
                           ::std::make_reverse_iterator(range.first), target);
    if(!qit)
      return nullopt;
    return data.rend() - *qit - 1;
  }

Opt_integer
std_array_rfind_if(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                   V_function predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second),
                                      ::std::make_reverse_iterator(range.first), predictor, true);
    if(!qit)
      return nullopt;
    return data.rend() - *qit - 1;
  }

Opt_integer
std_array_rfind_if_not(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                       V_function predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second),
                                      ::std::make_reverse_iterator(range.first), predictor, false);
    if(!qit)
      return nullopt;
    return data.rend() - *qit - 1;
  }

V_integer
std_array_count(V_array data, V_integer from, Opt_integer length, Value target)
  {
    int64_t count = 0;
    auto range = do_slice(data, from, length);
    while(auto qit = do_find_opt(range.first, range.second, target)) {
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

V_integer
std_array_count_if(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                   V_function predictor)
  {
    int64_t count = 0;
    auto range = do_slice(data, from, length);
    while(auto qit = do_find_if_opt(global, range.first, range.second, predictor, true)) {
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

V_integer
std_array_count_if_not(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                       V_function predictor)
  {
    int64_t count = 0;
    auto range = do_slice(data, from, length);
    while(auto qit = do_find_if_opt(global, range.first, range.second, predictor, false)) {
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

V_array
std_array_exclude(V_array data, V_integer from, Opt_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    ptrdiff_t dist = data.end() - range.second;
    while(auto qit = do_find_opt(range.first, range.second, target)) {
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return ::std::move(data);
  }

V_array
std_array_exclude_if(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                     V_function predictor)
  {
    auto range = do_slice(data, from, length);
    ptrdiff_t dist = data.end() - range.second;
    while(auto qit = do_find_if_opt(global, range.first, range.second, predictor, true)) {
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return ::std::move(data);
  }

V_array
std_array_exclude_if_not(Global_Context& global, V_array data, V_integer from, Opt_integer length,
                         V_function predictor)
  {
    auto range = do_slice(data, from, length);
    ptrdiff_t dist = data.end() - range.second;
    while(auto qit = do_find_if_opt(global, range.first, range.second, predictor, false)) {
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return ::std::move(data);
  }

V_boolean
std_array_is_sorted(Global_Context& global, V_array data, Opt_function comparator)
  {
    if(data.size() <= 1)
      // If `data` contains no more than 2 elements, it is considered sorted.
      return true;

    cow_vector<Reference> args;
    for(auto it = data.begin() + 1;  it != data.end();  ++it) {
      // Compare the two elements.
      auto cmp = do_compare(global, args, comparator, it[-1], it[0]);
      if((cmp == compare_greater) || (cmp == compare_unordered))
        return false;
    }
    return true;
  }

Opt_integer
std_array_binary_search(Global_Context& global, V_array data, Value target, Opt_function comparator)
  {
    cow_vector<Reference> args;
    auto pair = do_bsearch(global, args, data.begin(), data.end(), comparator, target);
    if(!pair.second)
      return nullopt;
    return pair.first - data.begin();
  }

V_integer
std_array_lower_bound(Global_Context& global, V_array data, Value target, Opt_function comparator)
  {
    cow_vector<Reference> args;
    auto lpos = do_bound(global, args, data.begin(), data.end(), comparator, target,
                         [](Compare cmp) { return cmp != compare_greater;  });
    return lpos - data.begin();
  }

V_integer
std_array_upper_bound(Global_Context& global, V_array data, Value target, Opt_function comparator)
  {
    cow_vector<Reference> args;
    auto upos = do_bound(global, args, data.begin(), data.end(), comparator, target,
                         [](Compare cmp) { return cmp == compare_less;  });
    return upos - data.begin();
  }

pair<V_integer, V_integer>
std_array_equal_range(Global_Context& global, V_array data, Value target, Opt_function comparator)
  {
    cow_vector<Reference> args;
    auto pair = do_bsearch(global, args, data.begin(), data.end(), comparator, target);
    auto lpos = do_bound(global, args, data.begin(), pair.first, comparator, target,
                         [](Compare cmp) { return cmp != compare_greater;  });
    auto upos = do_bound(global, args, pair.first, data.end(), comparator, target,
                         [](Compare cmp) { return cmp == compare_less;  });
    return ::std::make_pair(lpos - data.begin(), upos - lpos);
  }

V_array
std_array_sort(Global_Context& global, V_array data, Opt_function comparator)
  {
    if(data.size() <= 1)
      // Use reference counting as our advantage.
      return ::std::move(data);

    // The Merge Sort algorithm requires `O(n)` space.
    V_array temp(data.size());
    // Merge blocks of exponential sizes.
    cow_vector<Reference> args;
    ptrdiff_t bsize = 1;
    while(bsize < data.ssize()) {
      do_merge_blocks(temp, global, args, comparator, ::std::move(data), bsize, false);
      data.swap(temp);
      bsize *= 2;
    }
    return ::std::move(data);
  }

V_array
std_array_sortu(Global_Context& global, V_array data, Opt_function comparator)
  {
    if(data.size() <= 1)
      // Use reference counting as our advantage.
      return ::std::move(data);

    // The Merge Sort algorithm requires `O(n)` space.
    V_array temp(data.size());
    // Merge blocks of exponential sizes.
    cow_vector<Reference> args;
    ptrdiff_t bsize = 1;
    while(bsize * 2 < data.ssize()) {
      do_merge_blocks(temp, global, args, comparator, ::std::move(data), bsize, false);
      data.swap(temp);
      bsize *= 2;
    }
    auto epos = do_merge_blocks(temp, global, args, comparator, ::std::move(data), bsize, true);
    temp.erase(epos, temp.end());
    data.swap(temp);
    return ::std::move(data);
  }

Value
std_array_max_of(Global_Context& global, V_array data, Opt_function comparator)
  {
    auto qmax = data.begin();
    if(qmax == data.end())
      // Return `null` if `data` is empty.
      return V_null();

    // Compare `*qmax` with the other elements, ignoring unordered elements.
    cow_vector<Reference> args;
    for(auto it = qmax + 1;  it != data.end();  ++it)
      if(do_compare(global, args, comparator, *qmax, *it) == compare_less)
        qmax = it;
    return *qmax;
  }

Value
std_array_min_of(Global_Context& global, V_array data, Opt_function comparator)
  {
    auto qmin = data.begin();
    if(qmin == data.end())
      // Return `null` if `data` is empty.
      return V_null();

    // Compare `*qmin` with the other elements, ignoring unordered elements.
    cow_vector<Reference> args;
    for(auto it = qmin + 1;  it != data.end();  ++it)
      if(do_compare(global, args, comparator, *qmin, *it) == compare_greater)
        qmin = it;
    return *qmin;
  }

V_array
std_array_reverse(V_array data)
  {
    // This is an easy matter, isn't it?
    return V_array(data.move_rbegin(), data.move_rend());
  }

V_array
std_array_generate(Global_Context& global, V_function generator, V_integer length)
  {
    V_array data;
    data.reserve(static_cast<size_t>(length));
    cow_vector<Reference> args;
    for(int64_t i = 0;  i < length;  ++i) {
      // Set up arguments for the user-defined generator.
      args.resize(2);
      args.mut(0) = do_make_temporary(i);
      args.mut(1) = do_make_temporary(data.empty() ? null_value : data.back());

      // Call the generator function and push the return value.
      auto self = generator.invoke(global, ::std::move(args));
      data.emplace_back(self.read());
    }
    return data;
  }

V_array
std_array_shuffle(V_array data, Opt_integer seed)
  {
    if(data.size() <= 1)
      // Use reference counting as our advantage.
      return ::std::move(data);

    // Create a linear congruential generator.
    uint64_t lcg = seed ? static_cast<uint64_t>(*seed) : generate_random_seed();

    const auto bptr = data.mut_data();
    const auto eptr = bptr + data.ssize();

    // Shuffle elements.
    for(auto ps = bptr;  ps != eptr;  ++ps) {
      // These arguments are the same as glibc's `drand48()` function.
      //   https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
      lcg *= 0x5DEECE66D;     // a
      lcg += 0xB;             // c
      lcg &= 0xFFFFFFFFFFFF;  // m

      // Pick a random target.
      auto pd = ::rocket::get_probing_origin(bptr, eptr, static_cast<size_t>(lcg >> 16));
      if(ps != pd)
        swap(*ps, *pd);
    }
    return ::std::move(data);
  }

V_array
std_array_rotate(V_array data, V_integer shift)
  {
    if(data.size() <= 1)
      // Use reference counting as our advantage.
      return ::std::move(data);

    int64_t seek = shift % data.ssize();
    if(seek == 0)
      // Use reference counting as our advantage.
      return ::std::move(data);

    // Rotate it.
    seek = ((~seek >> 63) & data.ssize()) - seek;
    ::rocket::rotate(data.mut_data(), 0, static_cast<size_t>(seek), data.size());
    return ::std::move(data);
  }

V_array
std_array_copy_keys(V_object source)
  {
    V_array data;
    data.reserve(source.size());
    ::rocket::for_each(source, [&](const auto& p) { data.emplace_back(V_string(p.first));  });
    return data;
  }

V_array
std_array_copy_values(V_object source)
  {
    V_array data;
    data.reserve(source.size());
    ::rocket::for_each(source, [&](const auto& p) { data.emplace_back(p.second);  });
    return data;
  }

void
create_bindings_array(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.array.slice()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("slice"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.slice(data, from, [length])`

  * Copies a subrange of `data` to create a new array. Elements are
    copied from `from` if it is non-negative, or from
    `countof(data) + from` otherwise. If `length` is set to an
    integer, no more than this number of elements will be copied.
    If it is absent, all elements from `from` to the end of `data`
    will be copied. If `from` is outside `data`, an empty array
    is returned.

  * Returns the specified subarray of `data`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.slice"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    V_integer from;
    Opt_integer length;
    if(reader.I().v(data).v(from).o(length).F()) {
      Reference_root::S_temporary xref = { std_array_slice(::std::move(data), from, length) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.replace_slice()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("replace_slice"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.replace_slice(data, from, replacement)`

  * Replaces all elements from `from` to the end of `data` with
    `replacement` and returns the new array. If `from` is negative,
    it specifies an offset from the end of `data`. This function
    returns a new array without modifying `data`.

  * Returns a new array with the subrange replaced.

`std.array.replace_slice(data, from, [length], replacement)`

  * Replaces a subrange of `data` with `replacement` to create a
    new array. `from` specifies the start of the subrange to
    replace. If `from` is negative, it specifies an offset from the
    end of `data`. `length` specifies the maximum number of
    elements to replace. If it is set to `null`, this function is
    equivalent to `replace_slice(data, from, replacement)`. This
    function returns a new array without modifying `data`.

  * Returns a new array with the subrange replaced.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.replace"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    V_integer from;
    V_array replacement;
    if(reader.I().v(data).v(from).S(state).v(replacement).F()) {
      Reference_root::S_temporary xref = { std_array_replace_slice(::std::move(data), from, nullopt,
                                                                   ::std::move(replacement)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).v(replacement).F()) {
      Reference_root::S_temporary xref = { std_array_replace_slice(::std::move(data), from, length,
                                                                   ::std::move(replacement)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.find()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.find(data, target)`

  * Searches `data` for the first occurrence of `target`.

  * Returns the subscript of the first match of `target` in `data`
    if one is found, which is always non-negative, or `null`
    otherwise.

`std.array.find(data, from, target)`

  * Searches `data` for the first occurrence of `target`. The
    search operation is performed on the same subrange that would
    be returned by `slice(data, from)`.

  * Returns the subscript of the first match of `target` in `data`
    if one is found, which is always non-negative, or `null`
    otherwise.

`std.array.find(data, from, [length], target)`

  * Searches `data` for the first occurrence of `target`. The
    search operation is performed on the same subrange that would
    be returned by `slice(data, from, length)`.

  * Returns the subscript of the first match of `target` in `data`
    if one is found, which is always non-negative, or `null`
    otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.find"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    Value target;
    if(reader.I().v(data).S(state).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_find(::std::move(data), 0, nullopt,
                                                          ::std::move(target)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_find(::std::move(data), from, nullopt,
                                                          ::std::move(target)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_find(::std::move(data), from, length,
                                                          ::std::move(target)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.find_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_if"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.find_if(data, predictor)`

  * Finds the first element, namely `x`, in `data`, for which
    `predictor(x)` yields logically true.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.

`std.array.find_if(data, from, predictor)`

  * Finds the first element, namely `x`, in `data`, for which
    `predictor(x)` yields logically true. The search operation is
    performed on the same subrange that would be returned by
    `slice(data, from)`.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.

`std.array.find_if(data, from, [length], predictor)`

  * Finds the first element, namely `x`, in `data`, for which
    `predictor(x)` yields logically true. The search operation is
    performed on the same subrange that would be returned by
    `slice(data, from, length)`.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.find_if"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    V_function predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_find_if(global, ::std::move(data), 0, nullopt,
                                                             ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_find_if(global, ::std::move(data), from, nullopt,
                                                             ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_find_if(global, ::std::move(data), from, length,
                                                             ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.find_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_if_not"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.find_if_not(data, predictor)`

  * Finds the first element, namely `x`, in `data`, for which
    `predictor(x)` yields logically false.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.

`std.array.find_if_not(data, from, predictor)`

  * Finds the first element, namely `x`, in `data`, for which
    `predictor(x)` yields logically false. The search operation is
    performed on the same subrange that would be returned by
    `slice(data, from)`.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.

`std.array.find_if_not(data, from, [length], predictor)`

  * Finds the first element, namely `x`, in `data`, for which
    `predictor(x)` yields logically false. The search operation is
    performed on the same subrange that would be returned by
    `slice(data, from, length)`.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.find_if_not"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    V_function predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_find_if_not(global, ::std::move(data), 0, nullopt,
                                                                 ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_find_if_not(global, ::std::move(data), from, nullopt,
                                                                 ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_find_if_not(global, ::std::move(data), from, length,
                                                                 ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.rfind()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.rfind(data, target)`

  * Searches `data` for the last occurrence of `target`.

  * Returns the subscript of the last match of `target` in `data`
    if one is found, which is always non-negative, or `null`
    otherwise.

`std.array.rfind(data, from, target)`

  * Searches `data` for the last occurrence of `target`. The search
    operation is performed on the same subrange that would be
    returned by `slice(data, from)`.

  * Returns the subscript of the last match of `target` in `data`
    if one is found, which is always non-negative, or `null`
    otherwise.

`std.array.rfind(data, from, [length], target)`

  * Searches `data` for the last occurrence of `target`. The search
    operation is performed on the same subrange that would be
    returned by `slice(data, from, length)`.

  * Returns the subscript of the last match of `target` in `data`
    if one is found, which is always non-negative, or `null`
    otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.rfind"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    Value target;
    if(reader.I().v(data).S(state).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_rfind(::std::move(data), 0, nullopt,
                                                           ::std::move(target)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_rfind(::std::move(data), from, nullopt,
                                                           ::std::move(target)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_rfind(::std::move(data), from, length,
                                                           ::std::move(target)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.rfind_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind_if"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.rfind_if(data, predictor)`

  * Finds the last element, namely `x`, in `data`, for which
    `predictor(x)` yields logically true.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.

`std.array.rfind_if(data, from, predictor)`

  * Finds the last element, namely `x`, in `data`, for which
    `predictor(x)` yields logically true. The search operation is
    performed on the same subrange that would be returned by
    `slice(data, from)`.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.

`std.array.rfind_if(data, from, [length], predictor)`

  * Finds the last element, namely `x`, in `data`, for which
    `predictor(x)` yields logically true. The search operation is
    performed on the same subrange that would be returned by
    `slice(data, from, length)`.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.rfind_if"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    V_function predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_rfind_if(global, ::std::move(data), 0, nullopt,
                                                              ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_rfind_if(global, ::std::move(data), from, nullopt,
                                                              ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_rfind_if(global, ::std::move(data), from, length,
                                                              ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.rfind_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind_if_not"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.rfind_if_not(data, predictor)`

  * Finds the last element, namely `x`, in `data`, for which
    `predictor(x)` yields logically false.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.

`std.array.rfind_if_not(data, from, predictor)`

  * Finds the last element, namely `x`, in `data`, for which
    `predictor(x)` yields logically false. The search operation is
    performed on the same subrange that would be returned by
    `slice(data, from)`.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.

`std.array.rfind_if_not(data, from, [length], predictor)`

  * Finds the last element, namely `x`, in `data`, for which
    `predictor(x)` yields logically false. The search operation is
    performed on the same subrange that would be returned by
    `slice(data, from, length)`.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.rfind_if_not"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    V_function predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_rfind_if_not(global, ::std::move(data), 0, nullopt,
                                                                  ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_rfind_if_not(global, ::std::move(data), from, nullopt,
                                                                  ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_rfind_if_not(global, ::std::move(data), from, length,
                                                                  ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.count()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("count"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.count(data, target)`

  * Searches `data` for `target` and figures the total number of
    occurrences.

  * Returns the number of occurrences as an integer, which is
    always non-negative.

`std.array.count(data, from, target)`

  * Searches `data` for `target` and figures the total number of
    occurrences. The search operation is performed on the same
    subrange that would be returned by `slice(data, from)`.

  * Returns the number of occurrences as an integer, which is
    always non-negative.

`std.array.count(data, from, [length], target)`

  * Searches `data` for `target` and figures the total number of
    occurrences. The search operation is performed on the same
    subrange that would be returned by `slice(data, from, length)`.

  * Returns the number of occurrences as an integer, which is
    always non-negative.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.count"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    Value target;
    if(reader.I().v(data).S(state).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_count(::std::move(data), 0, nullopt,
                                                           ::std::move(target)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_count(::std::move(data), from, nullopt,
                                                           ::std::move(target)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_count(::std::move(data), from, length,
                                                           ::std::move(target)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.count_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("count_if"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.count_if(data, target, predictor)`

  * Searches `data` for every element, namely `x`, such that
    `predictor(x)` yields logically true, and figures the total
    number of such occurrences.

  * Returns the number of occurrences as an integer, which is
    always non-negative.

`std.array.count_if(data, from, target, predictor)`

  * Searches `data` for every element, namely `x`, such that
    `predictor(x)` yields logically true, and figures the total
    number of elements. The search operation is performed on the
    same subrange that would be returned by `slice(data, from)`.

  * Returns the number of occurrences as an integer, which is
    always non-negative.

`std.array.count_if(data, from, [length], target, predictor)`

  * Searches `data` for every element, namely `x`, such that
    `predictor(x)` yields logically true, and figures the total
    number of elements. The search operation is performed on the
    same subrange that would be returned by `slice(data, from,
    length)`.

  * Returns the number of occurrences as an integer, which is
    always non-negative.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.count_if"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    V_function predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_count_if(global, ::std::move(data), 0, nullopt,
                                                              ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_count_if(global, ::std::move(data), from, nullopt,
                                                              ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_count_if(global, ::std::move(data), from, length,
                                                              ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.count_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("count_if_not"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.count_if_not(data, target, predictor)`

  * Searches on every element, namely `x`, in `data`, such that
    `predictor(x)` yields logically false, and figures the total
    number of such occurrences.

  * Returns the number of occurrences as an integer, which is
    always non-negative.

`std.array.count_if_not(data, from, target, predictor)`

  * Searches on every element, namely `x`, in `data`, such that
    `predictor(x)` yields logically false, and figures the total
    number of elements. The search operation is performed on the
    same subrange that would be returned by `slice(data, from)`.

  * Returns the number of occurrences as an integer, which is
    always non-negative.

`std.array.count_if_not(data, from, [length], target, predictor)`

  * Searches on every element, namely `x`, in `data`, such that
    `predictor(x)` yields logically false, and figures the total
    number of elements. The search operation is performed on the
    same subrange that would be returned by `slice(data, from,
    length)`.

  * Returns the number of occurrences as an integer, which is
    always non-negative.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.count_if_not"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    V_function predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_count_if_not(global, ::std::move(data), 0, nullopt,
                                                                  ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_count_if_not(global, ::std::move(data), from, nullopt,
                                                                  ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_count_if_not(global, ::std::move(data), from, length,
                                                                  ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.exclude()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("exclude"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.exclude(data, target)`

  * Removes every element from `data` which compares equal to
    `target` to create a new array. This function returns a new
    array without modifying `data`.

  * Returns a new array with all occurrences removed.

`std.array.exclude(data, from, target)`

  * Removes every element from `data` which both compares equal
    to `target` and is within the same subrange that would be
    returned by `slice(data, from)` to create a new array. This
    function returns a new array without modifying `data`.

  * Returns a new array with all occurrences removed.

`std.array.exclude(data, from, [length], target)`

  * Removes every element from `data` which both compares equal
    to `target` and is within the same subrange that would be
    returned by `slice(data, from, length)` to create a new array.
    This function returns a new array without modifying `data`.

  * Returns a new array with all occurrences removed.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.exclude"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    Value target;
    if(reader.I().v(data).S(state).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_exclude(::std::move(data), 0, nullopt,
                                                             ::std::move(target)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_exclude(::std::move(data), from, nullopt,
                                                             ::std::move(target)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).o(target).F()) {
      Reference_root::S_temporary xref = { std_array_exclude(::std::move(data), from, length,
                                                             ::std::move(target)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.exclude_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("exclude_if"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.exclude_if(data, target, predictor)`

  * Removes every element from `data`, namely `x`, such that
    `predictor(x)` yields logically false, to create a new array.
    This function returns a new array without modifying `data`.

  * Returns a new array with all occurrences removed.

`std.array.exclude_if(data, from, target, predictor)`

  * Removes every element from `data`, namely `x`, such that
    both `predictor(x)` yields logically false and `x` is within
    the subrange that would be returned by `slice(data, from)`,
    to create a new array. This function returns a new array
    without modifying `data`.

  * Returns a new array with all occurrences removed.

`std.array.exclude_if(data, from, [length], target, predictor)`

  * Removes every element from `data`, namely `x`, such that
    both `predictor(x)` yields logically false and `x` is within
    the subrange that would be returned by `slice(data, from,
    length)`, to create a new array. This function returns a new
    array without modifying `data`.

  * Returns a new array with all occurrences removed.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.exclude_if"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    V_function predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_exclude_if(global, ::std::move(data), 0, nullopt,
                                                                ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_exclude_if(global, ::std::move(data), from, nullopt,
                                                                ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_exclude_if(global, ::std::move(data), from, length,
                                                                ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.exclude_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("exclude_if_not"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.exclude_if_not(data, target, predictor)`

  * Removes every element from `data`, namely `x`, such that
    `predictor(x)` yields logically true, to create a new array.
    This function returns a new array without modifying `data`.

  * Returns a new array with all occurrences removed.

`std.array.exclude_if_not(data, from, target, predictor)`

  * Removes every element from `data`, namely `x`, such that
    both `predictor(x)` yields logically true and `x` is within
    the subrange that would be returned by `slice(data, from)`,
    to create a new array. This function returns a new array
    without modifying `data`.

  * Returns a new array with all occurrences removed.

`std.array.exclude_if_not(data, from, [length], target, predictor)`

  * Removes every element from `data`, namely `x`, such that
    both `predictor(x)` yields logically true and `x` is within
    the subrange that would be returned by `slice(data, from,
    length)`, to create a new array. This function returns a new
    array without modifying `data`.

  * Returns a new array with all occurrences removed.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.exclude_if_not"), ::rocket::cref(args));
    Argument_Reader::State state;
    // Parse arguments.
    V_array data;
    V_function predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_exclude_if_not(global, ::std::move(data), 0, nullopt,
                                                                    ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_exclude_if_not(global, ::std::move(data), from, nullopt,
                                                                    ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    Opt_integer length;
    if(reader.L(state).o(length).v(predictor).F()) {
      Reference_root::S_temporary xref = { std_array_exclude_if_not(global, ::std::move(data), from, length,
                                                                    ::std::move(predictor)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.is_sorted()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("is_sorted"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.is_sorted(data, [comparator])`

  * Checks whether `data` is sorted. That is, there is no pair of
    adjacent elements in `data` such that the first one is greater
    than or unordered with the second one. Elements are compared
    using `comparator`, which shall be a binary function that
    returns a negative integer or real if the first argument is
    less than the second one, a positive integer or real if the
    first argument is greater than the second one, or `0` if the
    arguments are equal; other values indicate that the arguments
    are unordered. If no `comparator` is provided, the built-in
    3-way comparison operator is used. An array that contains no
    elements is considered to have been sorted.

  * Returns `true` if `data` is sorted or empty, or `false`
    otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.is_sorted"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Opt_function comparator;
    if(reader.I().v(data).o(comparator).F()) {
      Reference_root::S_temporary xref = { std_array_is_sorted(global, ::std::move(data),
                                                               ::std::move(comparator)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.binary_search()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("binary_search"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.binary_search(data, target, [comparator])`

  * Finds the first element in `data` that is equal to `target`.
    The principle of user-defined `comparator`s is the same as the
    `is_sorted()` function. As a consequence, the function call
    `is_sorted(data, comparator)` shall yield `true` prior to this
    call, otherwise the effect is undefined.

  * Returns the subscript of such an element as an integer, if one
    is found, or `null` otherwise.

  * Throws an exception if `data` has not been sorted properly. Be
    advised that in this case there is no guarantee whether an
    exception will be thrown or not.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.binary_search"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Value target;
    Opt_function comparator;
    if(reader.I().v(data).o(target).o(comparator).F()) {
      Reference_root::S_temporary xref = { std_array_binary_search(global, ::std::move(data), ::std::move(target),
                                                                   ::std::move(comparator)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.lower_bound()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("lower_bound"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.lower_bound(data, target, [comparator])`

  * Finds the first element in `data` that is greater than or equal
    to `target` and precedes all elements that are less than
    `target` if any. The principle of user-defined `comparator`s is
    the same as the `is_sorted()` function. As a consequence, the
    function call `is_sorted(data, comparator)` shall yield `true`
    prior to this call, otherwise the effect is undefined.

  * Returns the subscript of such an element as an integer. This
    function returns `countof(data)` if all elements are less than
    `target`.

  * Throws an exception if `data` has not been sorted properly. Be
    advised that in this case there is no guarantee whether an
    exception will be thrown or not.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.lower_bound"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Value target;
    Opt_function comparator;
    if(reader.I().v(data).o(target).o(comparator).F()) {
      Reference_root::S_temporary xref = { std_array_lower_bound(global, ::std::move(data), ::std::move(target),
                                                                 ::std::move(comparator)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.upper_bound()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("upper_bound"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.upper_bound(data, target, [comparator])`

  * Finds the first element in `data` that is greater than `target`
    and precedes all elements that are less than or equal to
    `target` if any. The principle of user-defined `comparator`s is
    the same as the `is_sorted()` function. As a consequence, the
    function call `is_sorted(data, comparator)` shall yield `true`
    prior to this call, otherwise the effect is undefined.

  * Returns the subscript of such an element as an integer. This
    function returns `countof(data)` if all elements are less than
    or equal to `target`.

  * Throws an exception if `data` has not been sorted properly. Be
    advised that in this case there is no guarantee whether an
    exception will be thrown or not.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.upper_bound"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Value target;
    Opt_function comparator;
    if(reader.I().v(data).o(target).o(comparator).F()) {
      Reference_root::S_temporary xref = { std_array_upper_bound(global, ::std::move(data), ::std::move(target),
                                                                 ::std::move(comparator)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.equal_range()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("equal_range"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.equal_range(data, target, [comparator])`

  * Gets the range of elements equivalent to `target` in `data` as
    a single function call. This function is equivalent to calling
    `lower_bound(data, target, comparator)` and
    `upper_bound(data, target, comparator)` respectively then
    storing the start and length in an array.

  * Returns an array of two integers, the first of which specifies
    the lower bound and the second of which specifies the number
    of elements in the range.

  * Throws an exception if `data` has not been sorted properly. Be
    advised that in this case there is no guarantee whether an
    exception will be thrown or not.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.lower_bound"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Value target;
    Opt_function comparator;
    if(reader.I().v(data).o(target).o(comparator).F()) {
      Reference_root::S_temporary xref = { std_array_equal_range(global, ::std::move(data), ::std::move(target),
                                                                 ::std::move(comparator)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.sort()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sort"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.sort(data, [comparator])`

  * Sorts elements in `data` in ascending order. The principle of
    user-defined `comparator`s is the same as the `is_sorted()`
    function. The algorithm shall finish in `O(n log n)` time where
    `n` is the number of elements in `data`, and shall be stable.
    This function returns a new array without modifying `data`.

  * Returns the sorted array.

  * Throws an exception if any elements are unordered. Be advised
    that in this case there is no guarantee whether an exception
    will be thrown or not.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.sort"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Opt_function comparator;
    if(reader.I().v(data).o(comparator).F()) {
      Reference_root::S_temporary xref = { std_array_sort(global, ::std::move(data), ::std::move(comparator)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.sortu()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sortu"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.sortu(data, [comparator])`

  * Sorts elements in `data` in ascending order, then removes all
    elements that have preceding equivalents. The principle of
    user-defined `comparator`s is the same as the `is_sorted()`
    function. The algorithm shall finish in `O(n log n)` time where
    `n` is the number of elements in `data`. This function returns
    a new array without modifying `data`.

  * Returns the sorted array with no duplicate elements.

  * Throws an exception if any elements are unordered. Be advised
    that in this case there is no guarantee whether an exception
    will be thrown or not.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.sortu"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Opt_function comparator;
    if(reader.I().v(data).o(comparator).F()) {
      Reference_root::S_temporary xref = { std_array_sortu(global, ::std::move(data), ::std::move(comparator)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.max_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("max_of"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.max_of(data, [comparator])`

  * Finds the maximum element in `data`. The principle of
    user-defined `comparator`s is the same as the `is_sorted()`
    function. Elements that are unordered with the first element
    are ignored silently.

  * Returns a copy of the maximum element, or `null` if `data` is
    empty.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.max_of"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Opt_function comparator;
    if(reader.I().v(data).o(comparator).F()) {
      Reference_root::S_temporary xref = { std_array_max_of(global, ::std::move(data), ::std::move(comparator)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.min_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("min_of"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.min_of(data, [comparator])`

  * Finds the minimum element in `data`. The principle of
    user-defined `comparator`s is the same as the `is_sorted()`
    function. Elements that are unordered with the first element
    are ignored silently.

  * Returns a copy of the minimum element, or `null` if `data` is
    empty.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.min_of"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Opt_function comparator;
    if(reader.I().v(data).o(comparator).F()) {
      Reference_root::S_temporary xref = { std_array_min_of(global, ::std::move(data), ::std::move(comparator)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.reverse()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("reverse"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.reverse(data)`

  * Reverses an array. This function returns a new array without
    modifying `data`.

  * Returns the reversed array.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.reverse"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Opt_function comparator;
    if(reader.I().v(data).o(comparator).F()) {
      Reference_root::S_temporary xref = { std_array_reverse(::std::move(data)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.generate()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("generate"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.generate(generator, length)`

  * Calls `generator` repeatedly up to `length` times and returns
    an array consisting of all values returned. `generator` shall
    be a binary function. The first argument will be the number of
    elements having been generated; the second argument is the
    previous element generated, or `null` in the case of the first
    element.

  * Returns an array containing all values generated.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& global) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.generate"), ::rocket::cref(args));
    // Parse arguments.
    V_function generator;
    V_integer length;
    if(reader.I().v(generator).v(length).F()) {
      Reference_root::S_temporary xref = { std_array_generate(global, ::std::move(generator), length) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.shuffle()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("shuffle"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.shuffle(data, [seed])`

  * Shuffles elements in `data` randomly. If `seed` is set to an
    integer, the internal pseudo random number generator will be
    initialized with it and will produce the same series of numbers
    for a specific `seed` value. If it is absent, an unspecified
    seed is generated when this function is called. This function
    returns a new array without modifying `data`.

  * Returns the shuffled array.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.shuffle"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    Opt_integer seed;
    if(reader.I().v(data).o(seed).F()) {
      Reference_root::S_temporary xref = { std_array_shuffle(::std::move(data), ::std::move(seed)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.rotate()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rotate"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.rotate(data, shift)`

  * Rotates elements in `data` by `shift`. That is, unless `data`
    is empty, the element at subscript `x` is moved to subscript
    `(x + shift) % countof(data)`. No element is added or removed.

  * Returns the rotated array. If `data` is empty, an empty array
    is returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.rotate"), ::rocket::cref(args));
    // Parse arguments.
    V_array data;
    V_integer shift;
    if(reader.I().v(data).v(shift).F()) {
      Reference_root::S_temporary xref = { std_array_rotate(::std::move(data), ::std::move(shift)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.copy_keys()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("copy_keys"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.copy_keys(source)`

  * Copies all keys from `source`, which shall be an object, to
    create an array.

  * Returns an array of all keys in `source`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.copy_keys"), ::rocket::cref(args));
    // Parse arguments.
    V_object source;
    if(reader.I().v(source).F()) {
      Reference_root::S_temporary xref = { std_array_copy_keys(::std::move(source)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.array.copy_values()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("copy_values"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.copy_values(source)`

  * Copies all values from `source`, which shall be an object, to
    create an array.

  * Returns an array of all values in `source`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::sref("std.array.copy_values"), ::rocket::cref(args));
    // Parse arguments.
    V_object source;
    if(reader.I().v(source).F()) {
      Reference_root::S_temporary xref = { std_array_copy_values(::std::move(source)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));
  }

}  // namespace asteria
