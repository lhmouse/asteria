// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "array.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../llds/reference_stack.hpp"
#include "../utils.hpp"

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
      // Behave like `::std::string::substr()` except that no exception is thrown when `from`
      // is greater than `data.size()`.
      if(from >= slen)
        return ::std::make_pair(data.end(), data.end());

      // Return a subrange from `begin() + from`.
      return do_slice(data, data.begin() + static_cast<ptrdiff_t>(from), length);
    }

    // Wrap `from` from the end. Notice that `from + slen` will not overflow when `from` is
    // negative and `slen` is not.
    auto rfrom = from + slen;
    if(rfrom >= 0)
      return do_slice(data, data.begin() + static_cast<ptrdiff_t>(rfrom), length);

    // Get a subrange from the beginning of `data`, if the wrapped index is before the first
    // byte.
    if(!length)
      return ::std::make_pair(data.begin(), data.end());

    if(*length <= 0)
      return ::std::make_pair(data.begin(), data.begin());

    // Get a subrange excluding the part before the beginning.
    // Notice that `rfrom + *length` will not overflow when `rfrom` is negative and `*length`
    // is not.
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

inline
Reference&
do_push_temporary(Reference_Stack& stack, const Value& value)
  {
    Reference::S_temporary xref = { value };
    return stack.emplace_back(::std::move(xref));
  }

template<typename IterT>
opt<IterT>
do_find_if_opt(Global_Context& global, IterT begin, IterT end, const V_function& pred, bool match)
  {
    Reference_Stack stack;
    for(auto it = ::std::move(begin);  it != end;  ++it) {
      // Set up arguments for the user-defined predictor.
      stack.clear();
      do_push_temporary(stack, *it);

      // Call the predictor function and check the return value.
      Reference self = Reference::S_constant();
      pred.invoke(self, global, ::std::move(stack));
      if(self.dereference_readonly().test() == match)
        return ::std::move(it);
    }
    // Fail to find an element.
    return nullopt;
  }

Compare
do_compare(Global_Context& global, Reference_Stack& stack,
           const Opt_function& kcomp, const Value& lhs, const Value& rhs)
  {
    // Use the builtin 3-way comparison operator if no comparator is provided.
    if(ROCKET_EXPECT(!kcomp))
      return lhs.compare(rhs);

    // Set up arguments for the user-defined comparator.
    stack.clear();
    do_push_temporary(stack, lhs);
    do_push_temporary(stack, rhs);

    // Call the predictor function and compare the result with `0`.
    Reference self = Reference::S_constant();
    kcomp.invoke(self, global, ::std::move(stack));
    return self.dereference_readonly().compare(V_integer(0));
  }

template<typename IterT>
pair<IterT, bool>
do_bsearch(Global_Context& global, Reference_Stack& stack, IterT begin, IterT end,
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
      auto cmp = do_compare(global, stack, kcomp, target, *mpos);
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
do_bound(Global_Context& global, Reference_Stack& stack, IterT begin, IterT end,
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
      auto cmp = do_compare(global, stack, kcomp, target, *mpos);
      if(cmp == compare_unordered)
        ASTERIA_THROW("Unordered elements (operands were `$1` and `$2`)", target, *mpos);

      if(pred(cmp))
        epos = mpos;
      else
        bpos = mpos + 1;
    }
  }

V_array::iterator&
do_merge_range(V_array::iterator& opos, Global_Context& global, Reference_Stack& stack,
               const Opt_function& kcomp, V_array::iterator ibegin, V_array::iterator iend,
               bool unique)
  {
    for(auto ipos = ibegin;  ipos != iend;  ++ipos)
      if(!unique || (do_compare(global, stack, kcomp, ipos[0], opos[-1]) != compare_equal))
        *(opos++) = ::std::move(*ipos);
    return opos;
  }

V_array::iterator
do_merge_blocks(V_array& output, Global_Context& global, Reference_Stack& stack,
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

      // Merge elements one by one, until either block has been exhausted, then store the index
      // of it here.
      size_t bi;
      for(;;) {
        auto cmp = do_compare(global, stack, kcomp, *(bpos[0]), *(bpos[1]));
        if(cmp == compare_unordered)
          ASTERIA_THROW("Unordered elements (operands were `$1` and `$2`)",
                        *(bpos[0]), *(bpos[1]));

        // For Merge Sort to be stable, the two elements will only be swapped if the first one
        // is greater than the second one.
        bi = (cmp == compare_greater);

        // Move this element unless uniqueness is requested and it is equal to the previous
        // output.
        bool discard = unique && (opos != output.begin())
                       && (do_compare(global, stack, kcomp, *(bpos[bi]), opos[-1]) == compare_equal);
        if(!discard)
          *(opos++) = ::std::move(*(bpos[bi]));
        bpos[bi]++;

        // When uniqueness is requested, if elements from the two blocks are equal, discard the one
        // from the second block. This may exhaust the second block.
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
      do_merge_range(opos, global, stack, kcomp, bpos[bi], bend[bi], unique);
    }
    // Copy all remaining elements.
    ROCKET_ASSERT(opos != output.begin());
    do_merge_range(opos, global, stack, kcomp, ipos, iend, unique);
    return opos;
  }

}  // namespace

V_array
std_array_slice(V_array data, V_integer from, Opt_integer length)
  {
    // Use reference counting as our advantage.
    V_array res = data;
    auto range = do_slice(res, from, length);
    if(range.second - range.first != res.ssize())
      res.assign(range.first, range.second);
    return res;
  }

V_array
std_array_replace_slice(V_array data, V_integer from, Opt_integer length,
                        V_array replacement, Opt_integer rfrom, Opt_integer rlength)
  {
    V_array res = data;
    auto range = do_slice(res, from, length);
    auto rep_range = do_slice(replacement, rfrom.value_or(0), rlength);

    // Convert iterators to subscripts.
    auto offset = ::std::distance(res.begin(), range.first);
    auto dist = ::std::distance(range.first, range.second);
    auto rep_offset = ::std::distance(replacement.begin(), rep_range.first);
    auto rep_dist = ::std::distance(rep_range.first, rep_range.second);

    if(dist >= replacement.ssize()) {
      // Overwrite the subrange in place.
      auto insp = ::std::move(replacement.mut_begin() + rep_offset,
                              replacement.mut_begin() + rep_offset + rep_dist,
                              res.mut_begin() + offset);

      res.erase(insp, res.begin() + offset + dist);
    }
    else {
      // Extend the range.
      auto insp = ::std::move(replacement.mut_begin() + rep_offset,
                              replacement.mut_begin() + rep_offset + dist,
                              res.mut_begin() + offset);

      res.insert(insp, replacement.move_begin() + rep_offset + dist,
                       replacement.move_begin() + rep_offset + rep_dist);
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

    Reference_Stack stack;
    for(auto it = data.begin() + 1;  it != data.end();  ++it) {
      // Compare the two elements.
      auto cmp = do_compare(global, stack, comparator, it[-1], it[0]);
      if((cmp == compare_greater) || (cmp == compare_unordered))
        return false;
    }
    return true;
  }

Opt_integer
std_array_binary_search(Global_Context& global, V_array data, Value target, Opt_function comparator)
  {
    Reference_Stack stack;
    auto pair = do_bsearch(global, stack, data.begin(), data.end(), comparator, target);
    if(!pair.second)
      return nullopt;
    return pair.first - data.begin();
  }

V_integer
std_array_lower_bound(Global_Context& global, V_array data, Value target, Opt_function comparator)
  {
    Reference_Stack stack;
    auto lpos = do_bound(global, stack, data.begin(), data.end(), comparator, target,
                         [](Compare cmp) { return cmp != compare_greater;  });
    return lpos - data.begin();
  }

V_integer
std_array_upper_bound(Global_Context& global, V_array data, Value target, Opt_function comparator)
  {
    Reference_Stack stack;
    auto upos = do_bound(global, stack, data.begin(), data.end(), comparator, target,
                         [](Compare cmp) { return cmp == compare_less;  });
    return upos - data.begin();
  }

pair<V_integer, V_integer>
std_array_equal_range(Global_Context& global, V_array data, Value target, Opt_function comparator)
  {
    Reference_Stack stack;
    auto pair = do_bsearch(global, stack, data.begin(), data.end(), comparator, target);
    auto lpos = do_bound(global, stack, data.begin(), pair.first, comparator, target,
                         [](Compare cmp) { return cmp != compare_greater;  });
    auto upos = do_bound(global, stack, pair.first, data.end(), comparator, target,
                         [](Compare cmp) { return cmp == compare_less;  });
    return ::std::make_pair(lpos - data.begin(), upos - lpos);
  }

V_array
std_array_sort(Global_Context& global, V_array data, Opt_function comparator)
  {
    if(data.size() <= 1)
      // Use reference counting as our advantage.
      return ::std::move(data);

    // Merge blocks of exponential sizes.
    V_array temp(data.size());
    Reference_Stack stack;
    ptrdiff_t bsize = 1;
    while(bsize < data.ssize()) {
      do_merge_blocks(temp, global, stack, comparator, ::std::move(data), bsize, false);
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

    // Merge blocks of exponential sizes.
    V_array temp(data.size());
    Reference_Stack stack;
    ptrdiff_t bsize = 1;
    while(bsize * 2 < data.ssize()) {
      do_merge_blocks(temp, global, stack, comparator, ::std::move(data), bsize, false);
      data.swap(temp);
      bsize *= 2;
    }
    auto epos = do_merge_blocks(temp, global, stack, comparator, ::std::move(data), bsize, true);
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
    Reference_Stack stack;
    for(auto it = qmax + 1;  it != data.end();  ++it)
      if(do_compare(global, stack, comparator, *qmax, *it) == compare_less)
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
    Reference_Stack stack;
    for(auto it = qmin + 1;  it != data.end();  ++it)
      if(do_compare(global, stack, comparator, *qmin, *it) == compare_greater)
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
    Reference_Stack stack;
    for(int64_t i = 0;  i < length;  ++i) {
      // Set up arguments for the user-defined generator.
      stack.clear();
      do_push_temporary(stack, i);
      do_push_temporary(stack, data.empty() ? null_value : data.back());

      // Call the generator function and push the return value.
      Reference self = Reference::S_constant();
      generator.invoke(self, global, ::std::move(stack));
      data.emplace_back(self.dereference_readonly());
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
    result.insert_or_assign(sref("slice"),
      ASTERIA_BINDING_BEGIN("std.array.slice", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;

        reader.start_overload();
        reader.required(data);    // data
        reader.required(from);    // from
        reader.optional(len);     // [length]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_slice, data, from, len);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("replace_slice"),
      ASTERIA_BINDING_BEGIN("std.array.replace_slice", self, global, reader) {
        V_array text;
        V_integer from;
        Opt_integer len;
        V_array rep;
        Opt_integer rfrom;
        Opt_integer rlen;

        reader.start_overload();
        reader.required(text);      // text
        reader.required(from);      // from
        reader.save_state(0);
        reader.required(rep);       // replacement
        reader.optional(rfrom);     // rep_from
        reader.optional(rlen);      // rep_length
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_replace_slice, text, from, nullopt, rep, rfrom, rlen);

        reader.load_state(0);       // text, from
        reader.optional(len);       // [length]
        reader.required(rep);       // replacement
        reader.optional(rfrom);     // rep_from
        reader.optional(rlen);      // rep_length
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_replace_slice, text, from, len, rep, rfrom, rlen);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("find"),
      ASTERIA_BINDING_BEGIN("std.array.find", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_find, data, 0, nullopt, targ);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_find, data, from, nullopt, targ);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_find, data, from, len, targ);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("find_if"),
      ASTERIA_BINDING_BEGIN("std.array.find_if", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        V_function pred;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_find_if, global, data, 0, nullopt, pred);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_find_if, global, data, from, nullopt, pred);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_find_if, global, data, from, len, pred);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("find_if_not"),
      ASTERIA_BINDING_BEGIN("std.array.find_if_not", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        V_function pred;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_find_if_not, global, data, 0, nullopt, pred);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_find_if_not, global, data, from, nullopt, pred);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_find_if_not, global, data, from, len, pred);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("rfind"),
      ASTERIA_BINDING_BEGIN("std.array.rfind", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_rfind, data, 0, nullopt, targ);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_rfind, data, from, nullopt, targ);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_rfind, data, from, len, targ);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("rfind_if"),
      ASTERIA_BINDING_BEGIN("std.array.rfind_if", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        V_function pred;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_rfind_if, global, data, 0, nullopt, pred);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_rfind_if, global, data, from, nullopt, pred);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_rfind_if, global, data, from, len, pred);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("rfind_if_not"),
      ASTERIA_BINDING_BEGIN("std.array.rfind_if_not", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        V_function pred;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_rfind_if_not, global, data, 0, nullopt, pred);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_rfind_if_not, global, data, from, nullopt, pred);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_rfind_if_not, global, data, from, len, pred);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("count"),
      ASTERIA_BINDING_BEGIN("std.array.count", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_count, data, 0, nullopt, targ);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_count, data, from, nullopt, targ);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_count, data, from, len, targ);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("count_if"),
      ASTERIA_BINDING_BEGIN("std.array.count_if", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        V_function pred;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_count_if, global, data, 0, nullopt, pred);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_count_if, global, data, from, nullopt, pred);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_count_if, global, data, from, len, pred);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("count_if_not"),
      ASTERIA_BINDING_BEGIN("std.array.count_if_not", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        V_function pred;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_count_if_not, global, data, 0, nullopt, pred);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_count_if_not, global, data, from, nullopt, pred);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_count_if_not, global, data, from, len, pred);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("exclude"),
      ASTERIA_BINDING_BEGIN("std.array.exclude", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_exclude, data, 0, nullopt, targ);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_exclude, data, from, nullopt, targ);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.optional(targ);     // [target]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_exclude, data, from, len, targ);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("exclude_if"),
      ASTERIA_BINDING_BEGIN("std.array.exclude_if", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        V_function pred;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_exclude_if, global, data, 0, nullopt, pred);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_exclude_if, global, data, from, nullopt, pred);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_exclude_if, global, data, from, len, pred);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("exclude_if_not"),
      ASTERIA_BINDING_BEGIN("std.array.exclude_if_not", self, global, reader) {
        V_array data;
        V_integer from;
        Opt_integer len;
        V_function pred;

        reader.start_overload();
        reader.required(data);     // data
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_exclude_if_not, global, data, 0, nullopt, pred);

        reader.load_state(0);      // data
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_exclude_if_not, global, data, from, nullopt, pred);

        reader.load_state(0);      // data, from
        reader.optional(len);      // [length]
        reader.required(pred);     // predictor
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_exclude_if_not, global, data, from, len, pred);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("is_sorted"),
      ASTERIA_BINDING_BEGIN("std.array.is_sorted", self, global, reader) {
        V_array data;
        Opt_function comp;

        reader.start_overload();
        reader.required(data);     // data
        reader.optional(comp);     // [comparator]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_is_sorted, global, data, comp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("binary_search"),
      ASTERIA_BINDING_BEGIN("std.array.binary_search", self, global, reader) {
        V_array data;
        Value targ;
        Opt_function comp;

        reader.start_overload();
        reader.required(data);     // data
        reader.optional(targ);     // [target]
        reader.optional(comp);     // [comparator]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_binary_search, global, data, targ, comp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("lower_bound"),
      ASTERIA_BINDING_BEGIN("std.array.lower_bound", self, global, reader) {
        V_array data;
        Value targ;
        Opt_function comp;

        reader.start_overload();
        reader.required(data);     // data
        reader.optional(targ);     // [target]
        reader.optional(comp);     // [comparator]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_lower_bound, global, data, targ, comp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("upper_bound"),
      ASTERIA_BINDING_BEGIN("std.array.upper_bound", self, global, reader) {
        V_array data;
        Value targ;
        Opt_function comp;

        reader.start_overload();
        reader.required(data);     // data
        reader.optional(targ);     // [target]
        reader.optional(comp);     // [comparator]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_upper_bound, global, data, targ, comp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("equal_range"),
      ASTERIA_BINDING_BEGIN("std.array.equal_range", self, global, reader) {
        V_array data;
        Value targ;
        Opt_function comp;

        reader.start_overload();
        reader.required(data);     // data
        reader.optional(targ);     // [target]
        reader.optional(comp);     // [comparator]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_equal_range, global, data, targ, comp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("sort"),
      ASTERIA_BINDING_BEGIN("std.array.sort", self, global, reader) {
        V_array data;
        Opt_function comp;

        reader.start_overload();
        reader.required(data);     // data
        reader.optional(comp);     // [comparator]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_sort, global, data, comp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("sortu"),
      ASTERIA_BINDING_BEGIN("std.array.sortu", self, global, reader) {
        V_array data;
        Opt_function comp;

        reader.start_overload();
        reader.required(data);     // data
        reader.optional(comp);     // [comparator]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_sortu, global, data, comp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("max_of"),
      ASTERIA_BINDING_BEGIN("std.array.max_of", self, global, reader) {
        V_array data;
        Opt_function comp;

        reader.start_overload();
        reader.required(data);     // data
        reader.optional(comp);     // [comparator]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_max_of, global, data, comp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("min_of"),
      ASTERIA_BINDING_BEGIN("std.array.min_of", self, global, reader) {
        V_array data;
        Opt_function comp;

        reader.start_overload();
        reader.required(data);     // data
        reader.optional(comp);     // [comparator]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_min_of, global, data, comp);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("reverse"),
      ASTERIA_BINDING_BEGIN("std.array.reverse", self, global, reader) {
        V_array data;

        reader.start_overload();
        reader.required(data);     // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_reverse, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("generate"),
      ASTERIA_BINDING_BEGIN("std.array.generate", self, global, reader) {
        V_function gen;
        V_integer len;

        reader.start_overload();
        reader.required(gen);      // generator
        reader.required(len);      // length
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_generate, global, gen, len);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("shuffle"),
      ASTERIA_BINDING_BEGIN("std.array.shuffle", self, global, reader) {
        V_array data;
        Opt_integer seed;

        reader.start_overload();
        reader.required(data);     // data
        reader.optional(seed);     // [seed]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_shuffle, data, seed);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("rotate"),
      ASTERIA_BINDING_BEGIN("std.array.rotate", self, global, reader) {
        V_array data;
        V_integer shift;

        reader.start_overload();
        reader.required(data);     // data
        reader.required(shift);    // shift
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_rotate, data, shift);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("copy_keys"),
      ASTERIA_BINDING_BEGIN("std.array.copy_keys", self, global, reader) {
        V_object obj;

        reader.start_overload();
        reader.required(obj);    // source
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_copy_keys, obj);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(sref("copy_values"),
      ASTERIA_BINDING_BEGIN("std.array.copy_values", self, global, reader) {
        V_object obj;

        reader.start_overload();
        reader.required(obj);    // source
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_array_copy_values, obj);
      }
      ASTERIA_BINDING_END);
  }

}  // namespace asteria
