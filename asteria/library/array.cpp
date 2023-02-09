// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "array.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/global_context.hpp"
#include "../llds/reference_stack.hpp"
#include "../utils.hpp"
namespace asteria {
namespace {

pair<V_array::const_iterator, V_array::const_iterator>
do_slice(const V_array& data, V_array::const_iterator tbegin, const optV_integer& length)
  {
    if(!length || (*length >= data.end() - tbegin))
      return ::std::make_pair(tbegin, data.end());

    if(*length <= 0)
      return ::std::make_pair(tbegin, tbegin);

    // Don't go past the end.
    return ::std::make_pair(tbegin, tbegin + static_cast<ptrdiff_t>(*length));
  }

pair<V_array::const_iterator, V_array::const_iterator>
do_slice(const V_array& data, const V_integer& from, const optV_integer& length)
  {
    // Behave like `::std::string::substr()` except that no exception is
    // thrown when `from` is greater than `data.size()`.
    auto slen = static_cast<int64_t>(data.size());
    if(from >= slen)
      return ::std::make_pair(data.end(), data.end());

    // For positive offsets, return a subrange from `begin() + from`.
    if(from >= 0)
      return do_slice(data, data.begin() + static_cast<ptrdiff_t>(from), length);

    // Wrap `from` from the end. Notice that `from + slen` will not overflow
    // when `from` is negative and `slen` is not.
    auto rfrom = from + slen;
    if(rfrom >= 0)
      return do_slice(data, data.begin() + static_cast<ptrdiff_t>(rfrom), length);

    // Get a subrange from the beginning of `data`, if the wrapped index is
    // before the first byte.
    if(!length)
      return ::std::make_pair(data.begin(), data.end());

    if(*length <= 0)
      return ::std::make_pair(data.begin(), data.begin());

    // Get a subrange excluding the part before the beginning.
    // Notice that `rfrom + *length` will not overflow when `rfrom` is
    // negative and `*length` is not.
    return do_slice(data, data.begin(), rfrom + *length);
  }

template<typename IterT>
opt<IterT>
do_find_opt(Global_Context& global, Reference_Stack& stack,
            IterT begin, IterT end, const Value& target, bool match)
  {
    for(auto it = ::std::move(begin);  it != end;  ++it) {
      bool result;
      if(target.is_function()) {
        // `target` is a unary predictor.
        stack.clear();
        stack.push().set_temporary(*it);

        Reference self;
        self.set_temporary(nullopt);
        target.as_function().invoke(self, global, ::std::move(stack));
        result = self.dereference_readonly().test();
      }
      else {
        // `target` is a plain value.
        result = it->compare(target) == compare_equal;
      }
      if(result == match)
        return ::std::move(it);
    }
    return nullopt;
  }

Compare
do_compare(Global_Context& global, Reference_Stack& stack,
           const optV_function& kcomp, const Value& lhs, const Value& rhs)
  {
    // Use the builtin 3-way comparison operator if no comparator is provided.
    if(ROCKET_EXPECT(!kcomp))
      return lhs.compare(rhs);

    // Set up arguments for the user-defined comparator.
    stack.clear();
    stack.push().set_temporary(lhs);
    stack.push().set_temporary(rhs);

    // Call the predictor function and compare the result with `0`.
    Reference self;
    self.set_temporary(nullopt);
    kcomp.invoke(self, global, ::std::move(stack));
    return self.dereference_readonly().compare(V_integer(0));
  }

template<typename IterT>
pair<IterT, bool>
do_bsearch(Global_Context& global, Reference_Stack& stack, IterT begin, IterT end,
           const optV_function& kcomp, const Value& target)
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
        ASTERIA_THROW_RUNTIME_ERROR((
            "Unordered elements (operands were `$1` and `$2`)"),
            target, *mpos);

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
         const optV_function& kcomp, const Value& target, PredT&& pred)
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
        ASTERIA_THROW_RUNTIME_ERROR((
            "Unordered elements (operands were `$1` and `$2`)"),
            target, *mpos);

      if(pred(cmp))
        epos = mpos;
      else
        bpos = mpos + 1;
    }
  }

template<typename ComparatorT>
V_array&
do_merge_blocks(V_array& output, bool unique, V_array& input, ComparatorT&& compare, ptrdiff_t bsize)
  {
    ROCKET_ASSERT(output.size() >= input.size());
    ROCKET_ASSERT(bsize > 0);
    ROCKET_ASSERT(input.size() >= (size_t) bsize);

    // Merge adjacent blocks of `bsize` elements.
    auto bout = output.mut_begin();
    auto bin = input.mut_begin();
    auto ein = input.mut_end();

    auto output_element = [&](Value& elem)
      {
        // Check for duplicates.
        if(unique && (bout != output.begin())) {
          auto cmp = compare(bout[-1], elem);
          if(cmp == compare_unordered)
            ASTERIA_THROW_RUNTIME_ERROR((
                "Elements not comparable (operands were `$1` and `$2`)"),
                bout[-1], elem);

          if(cmp == compare_equal)
            return;
        }

        // Move the element now.
        bout += 1;
        bout[-1] = ::std::move(elem);
      };

    // Input data are divided into blocks of `bsize` elements.
    // First, repeat if there are at least two blocks.
    while(ein - bin > bsize) {
      V_array::iterator bpos[2];  // beginnings of blocks
      V_array::iterator epos[2];  // ends of blocks
      size_t tb;  // index of block that has just been taken

      bpos[0] = bin;
      bin += bsize;
      bpos[1] = bin;

      epos[0] = bin;
      bin += ::rocket::min(ein - bin, bsize);
      epos[1] = bin;

      do {
        // Compare the first elements from both blocks.
        auto cmp = compare(bpos[0][0], bpos[1][0]);
        if(cmp == compare_unordered)
          ASTERIA_THROW_RUNTIME_ERROR((
              "Elements not comparable (operands were `$1` and `$2`)"),
              bpos[0][0], bpos[1][0]);

        // Move the smaller element to the output array. For merge sorts to be
        // stable, if the elements compare equal, the first one is taken.
        tb = cmp == compare_greater;
        bpos[tb] += 1;
        output_element(bpos[tb][-1]);
      }
      while(bpos[tb] != epos[tb]);

      // The `tb` block has been exhausted. Move elements from the other block.
      tb ^= 1;
      ROCKET_ASSERT(bpos[tb] != epos[tb]);

      do {
        bpos[tb] += 1;
        output_element(bpos[tb][-1]);
      }
      while(bpos[tb] != epos[tb]);
    }

    // Move all remaining elements.
    while(bin != ein) {
      bin += 1;
      output_element(bin[-1]);
    }

    // Erase invalid elements.
    output.erase(bout, output.end());
    return output;
  }

}  // namespace

V_array
std_array_slice(V_array data, V_integer from, optV_integer length)
  {
    // Use reference counting as our advantage.
    V_array res = data;
    auto range = do_slice(res, from, length);
    if(range.second - range.first != res.ssize())
      res.assign(range.first, range.second);
    return res;
  }

V_array
std_array_replace_slice(V_array data, V_integer from, optV_integer length,
                        V_array replacement, optV_integer rfrom, optV_integer rlength)
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

optV_integer
std_array_find(Global_Context& global, V_array data, V_integer from, optV_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    Reference_Stack stack;
    auto qit = do_find_opt(global, stack, range.first, range.second, target, true);
    return qit ? (*qit - data.begin()) : optV_integer();
  }

optV_integer
std_array_find_not(Global_Context& global, V_array data, V_integer from, optV_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    Reference_Stack stack;
    auto qit = do_find_opt(global, stack, range.first, range.second, target, false);
    return qit ? (*qit - data.begin()) : optV_integer();
  }

optV_integer
std_array_rfind(Global_Context& global, V_array data, V_integer from, optV_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    Reference_Stack stack;
    auto qit = do_find_opt(global, stack,
                           ::std::make_reverse_iterator(range.second),
                           ::std::make_reverse_iterator(range.first),
                           target, true);
    return qit ? (data.rend() - *qit - 1) : optV_integer();
  }

optV_integer
std_array_rfind_not(Global_Context& global, V_array data, V_integer from, optV_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    Reference_Stack stack;
    auto qit = do_find_opt(global, stack,
                           ::std::make_reverse_iterator(range.second),
                           ::std::make_reverse_iterator(range.first),
                           target, false);
    return qit ? (data.rend() - *qit - 1) : optV_integer();
  }

V_integer
std_array_count(Global_Context& global, V_array data, V_integer from, optV_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    V_integer count = 0;
    Reference_Stack stack;
    while(auto qit = do_find_opt(global, stack, range.first, range.second, target, true)) {
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

V_integer
std_array_count_not(Global_Context& global, V_array data, V_integer from, optV_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    V_integer count = 0;
    Reference_Stack stack;
    while(auto qit = do_find_opt(global, stack, range.first, range.second, target, false)) {
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

V_array
std_array_exclude(Global_Context& global, V_array data, V_integer from, optV_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    ptrdiff_t dist = data.end() - range.second;
    Reference_Stack stack;
    while(auto qit = do_find_opt(global, stack, range.first, range.second, target, true)) {
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

V_array
std_array_exclude_not(Global_Context& global, V_array data, V_integer from, optV_integer length, Value target)
  {
    auto range = do_slice(data, from, length);
    ptrdiff_t dist = data.end() - range.second;
    Reference_Stack stack;
    while(auto qit = do_find_opt(global, stack, range.first, range.second, target, false)) {
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

V_boolean
std_array_is_sorted(Global_Context& global, V_array data, optV_function comparator)
  {
    if(data.size() == 0)
      return true;

    // The first element shall not be unordered with itself.
    Reference_Stack stack;
    auto comp = do_compare(global, stack, comparator, data.front(), data.front());
    if(comp != compare_equal)
      return false;

    if(data.size() <= 1)
      return true;

    // Check remaining elements.
    for(size_t k = 1;  k < data.size();  ++k) {
      comp = do_compare(global, stack, comparator, data.at(k - 1), data.at(k));
      if((comp != compare_less) && (comp != compare_equal))
        return false;
    }
    return true;
  }

optV_integer
std_array_binary_search(Global_Context& global, V_array data, Value target, optV_function comparator)
  {
    Reference_Stack stack;
    auto pair = do_bsearch(global, stack, data.begin(), data.end(), comparator, target);
    return pair.second ? (pair.first - data.begin()) : optV_integer();
  }

V_integer
std_array_lower_bound(Global_Context& global, V_array data, Value target, optV_function comparator)
  {
    Reference_Stack stack;
    auto lpos = do_bound(global, stack, data.begin(), data.end(), comparator, target,
                         [](Compare cmp) { return cmp != compare_greater;  });
    return lpos - data.begin();
  }

V_integer
std_array_upper_bound(Global_Context& global, V_array data, Value target, optV_function comparator)
  {
    Reference_Stack stack;
    auto upos = do_bound(global, stack, data.begin(), data.end(), comparator, target,
                         [](Compare cmp) { return cmp == compare_less;  });
    return upos - data.begin();
  }

pair<V_integer, V_integer>
std_array_equal_range(Global_Context& global, V_array data, Value target, optV_function comparator)
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
std_array_sort(Global_Context& global, V_array data, optV_function comparator)
  {
    // Use reference counting as our advantage.
    if(data.size() <= 1)
      return data;

    // Merge blocks of exponential sizes.
    Reference_Stack stack;
    auto compare = [&](const Value& lhs, const Value& rhs)
        { return do_compare(global, stack, comparator, lhs, rhs);  };

    V_array temp(data.size());
    ptrdiff_t bsize = 1;
    while(bsize < data.ssize()) {
      do_merge_blocks(temp, false, data, compare, bsize);
      data.swap(temp);
      bsize *= 2;
    }
    return data;
  }

V_array
std_array_sortu(Global_Context& global, V_array data, optV_function comparator)
  {
    // Use reference counting as our advantage.
    if(data.size() <= 1)
      return data;

    // Merge blocks of exponential sizes.
    Reference_Stack stack;
    auto compare = [&](const Value& lhs, const Value& rhs)
        { return do_compare(global, stack, comparator, lhs, rhs);  };

    V_array temp(data.size());
    ptrdiff_t bsize = 1;
    while(bsize * 2 < data.ssize()) {
      do_merge_blocks(temp, false, data, compare, bsize);
      data.swap(temp);
      bsize *= 2;
    }
    do_merge_blocks(temp, true, data, compare, bsize);
    return temp;
  }

V_array
std_array_ksort(Global_Context& global, V_object object, optV_function comparator)
  {
    // Collect key-value pairs.
    V_array data;
    data.reserve(object.size());

    for(const auto& r : object) {
      V_array pair(2);
      pair.mut(0) = r.first.rdstr();
      pair.mut(1) = r.second;
      data.emplace_back(::std::move(pair));
    }

    // Use reference counting as our advantage.
    if(data.size() <= 1)
      return data;

    // Merge blocks of exponential sizes. Keys are known to be unique.
    Reference_Stack stack;
    auto compare = [&](const Value& lhs, const Value& rhs)
        { return do_compare(global, stack, comparator,
                            lhs.as_array().at(0), rhs.as_array().at(0));  };

    V_array temp(data.size());
    ptrdiff_t bsize = 1;
    while(bsize < data.ssize()) {
      do_merge_blocks(temp, false, data, compare, bsize);
      data.swap(temp);
      bsize *= 2;
    }
    return data;
  }

Value
std_array_max_of(Global_Context& global, V_array data, optV_function comparator)
  {
    // Return `null` if `data` is empty.
    auto qmax = data.begin();
    if(qmax == data.end())
      return nullopt;

    // Compare `*qmax` with the other elements, ignoring unordered elements.
    Reference_Stack stack;
    for(auto it = qmax + 1;  it != data.end();  ++it)
      if(do_compare(global, stack, comparator, *qmax, *it) == compare_less)
        qmax = it;
    return *qmax;
  }

Value
std_array_min_of(Global_Context& global, V_array data, optV_function comparator)
  {
    // Return `null` if `data` is empty.
    auto qmin = data.begin();
    if(qmin == data.end())
      return nullopt;

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
    Reference self;
    Reference_Stack stack;
    for(int64_t i = 0;  i < length;  ++i) {
      // Set up arguments for the user-defined generator.
      stack.clear();
      stack.push().set_temporary(i);
      if(data.empty())
        stack.push().set_temporary(nullopt);
      else
        stack.push().set_temporary(data.back());

      // Call the generator function and push the return value.
      self.set_temporary(nullopt);
      generator.invoke(self, global, ::std::move(stack));
      data.emplace_back(self.dereference_readonly());
    }
    return data;
  }

V_array
std_array_shuffle(V_array data, optV_integer seed)
  {
    // Use reference counting as our advantage.
    if(data.size() <= 1)
      return data;

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
    return data;
  }

V_array
std_array_rotate(V_array data, V_integer shift)
  {
    // Use reference counting as our advantage.
    if(data.size() <= 1)
      return data;

    int64_t seek = shift % data.ssize();
    if(seek == 0)
      return data;

    // Rotate it.
    seek = ((~seek >> 63) & data.ssize()) - seek;
    ::rocket::rotate(data.mut_data(), 0, static_cast<size_t>(seek), data.size());
    return data;
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
      ASTERIA_BINDING(
        "std.array.slice", "data, from, [length]",
        Argument_Reader&& reader)
      {
        V_array data;
        V_integer from;
        optV_integer len;

        reader.start_overload();
        reader.required(data);
        reader.required(from);
        reader.optional(len);
        if(reader.end_overload())
          return (Value) std_array_slice(data, from, len);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("replace_slice"),
      ASTERIA_BINDING(
        "std.array.replace_slice", "data, from, [length], replacement, [rfrom, [rlength]]",
        Argument_Reader&& reader)
      {
        V_array text;
        V_integer from;
        optV_integer len;
        V_array rep;
        optV_integer rfrom;
        optV_integer rlen;

        reader.start_overload();
        reader.required(text);
        reader.required(from);
        reader.save_state(0);
        reader.required(rep);
        reader.optional(rfrom);
        reader.optional(rlen);
        if(reader.end_overload())
          return (Value) std_array_replace_slice(text, from, nullopt, rep, rfrom, rlen);

        reader.load_state(0);
        reader.optional(len);
        reader.required(rep);
        reader.optional(rfrom);
        reader.optional(rlen);
        if(reader.end_overload())
          return (Value) std_array_replace_slice(text, from, len, rep, rfrom, rlen);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("find"),
      ASTERIA_BINDING(
        "std.array.find", "data, [from, [length]], [target]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        V_integer from;
        optV_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_find(global, data, 0, nullopt, targ);

        reader.load_state(0);
        reader.required(from);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_find(global, data, from, nullopt, targ);

        reader.load_state(0);
        reader.optional(len);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_find(global, data, from, len, targ);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("find_not"),
      ASTERIA_BINDING(
        "std.array.find_not", "data, [from, [length]], [target]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        V_integer from;
        optV_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_find_not(global, data, 0, nullopt, targ);

        reader.load_state(0);
        reader.required(from);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_find_not(global, data, from, nullopt, targ);

        reader.load_state(0);
        reader.optional(len);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_find_not(global, data, from, len, targ);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("rfind"),
      ASTERIA_BINDING(
        "std.array.rfind", "data, [from, [length]], [target]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        V_integer from;
        optV_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_rfind(global, data, 0, nullopt, targ);

        reader.load_state(0);
        reader.required(from);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_rfind(global, data, from, nullopt, targ);

        reader.load_state(0);
        reader.optional(len);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_rfind(global, data, from, len, targ);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("rfind_not"),
      ASTERIA_BINDING(
        "std.array.rfind_not", "data, [from, [length]], [target]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        V_integer from;
        optV_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_rfind_not(global, data, 0, nullopt, targ);

        reader.load_state(0);
        reader.required(from);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_rfind_not(global, data, from, nullopt, targ);

        reader.load_state(0);
        reader.optional(len);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_rfind_not(global, data, from, len, targ);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("count"),
      ASTERIA_BINDING(
        "std.array.count", "data, [from, [length]], [target]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        V_integer from;
        optV_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_count(global, data, 0, nullopt, targ);

        reader.load_state(0);
        reader.required(from);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_count(global, data, from, nullopt, targ);

        reader.load_state(0);
        reader.optional(len);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_count(global, data, from, len, targ);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("count_not"),
      ASTERIA_BINDING(
        "std.array.count_not", "data, [from, [length]], [target]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        V_integer from;
        optV_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_count_not(global, data, 0, nullopt, targ);

        reader.load_state(0);
        reader.required(from);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_count_not(global, data, from, nullopt, targ);

        reader.load_state(0);
        reader.optional(len);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_count_not(global, data, from, len, targ);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("exclude"),
      ASTERIA_BINDING(
        "std.array.exclude", "data, [from, [length]], [target]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        V_integer from;
        optV_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_exclude(global, data, 0, nullopt, targ);

        reader.load_state(0);
        reader.required(from);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_exclude(global, data, from, nullopt, targ);

        reader.load_state(0);
        reader.optional(len);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_exclude(global, data, from, len, targ);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("exclude_not"),
      ASTERIA_BINDING(
        "std.array.exclude_not", "data, [from, [length]], [target]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        V_integer from;
        optV_integer len;
        Value targ;

        reader.start_overload();
        reader.required(data);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_exclude_not(global, data, 0, nullopt, targ);

        reader.load_state(0);
        reader.required(from);
        reader.save_state(0);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_exclude_not(global, data, from, nullopt, targ);

        reader.load_state(0);
        reader.optional(len);
        reader.optional(targ);
        if(reader.end_overload())
          return (Value) std_array_exclude_not(global, data, from, len, targ);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("is_sorted"),
      ASTERIA_BINDING(
        "std.array.is_sorted", "data, [comparator]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        optV_function comp;

        reader.start_overload();
        reader.required(data);
        reader.optional(comp);
        if(reader.end_overload())
          return (Value) std_array_is_sorted(global, data, comp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("binary_search"),
      ASTERIA_BINDING(
        "std.array.binary_search", "data, [target], [comparator]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        Value targ;
        optV_function comp;

        reader.start_overload();
        reader.required(data);
        reader.optional(targ);
        reader.optional(comp);
        if(reader.end_overload())
          return (Value) std_array_binary_search(global, data, targ, comp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("lower_bound"),
      ASTERIA_BINDING(
        "std.array.lower_bound", "data, [target], [comparator]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        Value targ;
        optV_function comp;

        reader.start_overload();
        reader.required(data);
        reader.optional(targ);
        reader.optional(comp);
        if(reader.end_overload())
          return (Value) std_array_lower_bound(global, data, targ, comp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("upper_bound"),
      ASTERIA_BINDING(
        "std.array.upper_bound", "data, [target], [comparator]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        Value targ;
        optV_function comp;

        reader.start_overload();
        reader.required(data);
        reader.optional(targ);
        reader.optional(comp);
        if(reader.end_overload())
          return (Value) std_array_upper_bound(global, data, targ, comp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("equal_range"),
      ASTERIA_BINDING(
        "std.array.equal_range", "data, [target], [comparator]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        Value targ;
        optV_function comp;

        reader.start_overload();
        reader.required(data);
        reader.optional(targ);
        reader.optional(comp);
        if(reader.end_overload())
          return (Value) std_array_equal_range(global, data, targ, comp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sort"),
      ASTERIA_BINDING(
        "std.array.sort", "data, [comparator]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        optV_function comp;

        reader.start_overload();
        reader.required(data);
        reader.optional(comp);
        if(reader.end_overload())
          return (Value) std_array_sort(global, data, comp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("sortu"),
      ASTERIA_BINDING(
        "std.array.sortu", "data, [comparator]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        optV_function comp;

        reader.start_overload();
        reader.required(data);
        reader.optional(comp);
        if(reader.end_overload())
          return (Value) std_array_sortu(global, data, comp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("ksort"),
      ASTERIA_BINDING(
        "std.array.ksort", "object, [comparator]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_object object;
        optV_function comp;

        reader.start_overload();
        reader.required(object);
        reader.optional(comp);
        if(reader.end_overload())
          return (Value) std_array_ksort(global, object, comp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("max_of"),
      ASTERIA_BINDING(
        "std.array.max_of", "data, [comparator]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        optV_function comp;

        reader.start_overload();
        reader.required(data);
        reader.optional(comp);
        if(reader.end_overload())
          return (Value) std_array_max_of(global, data, comp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("min_of"),
      ASTERIA_BINDING(
        "std.array.min_of", "data, [comparator]",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_array data;
        optV_function comp;

        reader.start_overload();
        reader.required(data);
        reader.optional(comp);
        if(reader.end_overload())
          return (Value) std_array_min_of(global, data, comp);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("reverse"),
      ASTERIA_BINDING(
        "std.array.reverse", "data",
        Argument_Reader&& reader)
      {
        V_array data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_array_reverse(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("generate"),
      ASTERIA_BINDING(
        "std.array.generate", "generator, length",
        Global_Context& global, Argument_Reader&& reader)
      {
        V_function gen;
        V_integer len;

        reader.start_overload();
        reader.required(gen);
        reader.required(len);
        if(reader.end_overload())
          return (Value) std_array_generate(global, gen, len);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("shuffle"),
      ASTERIA_BINDING(
        "std.array.shuffle", "data, [seed]",
        Argument_Reader&& reader)
      {
        V_array data;
        optV_integer seed;

        reader.start_overload();
        reader.required(data);
        reader.optional(seed);
        if(reader.end_overload())
          return (Value) std_array_shuffle(data, seed);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("rotate"),
      ASTERIA_BINDING(
        "std.array.rotate", "data, shift",
        Argument_Reader&& reader)
      {
        V_array data;
        V_integer shift;

        reader.start_overload();
        reader.required(data);
        reader.required(shift);
        if(reader.end_overload())
          return (Value) std_array_rotate(data, shift);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("copy_keys"),
      ASTERIA_BINDING(
        "std.array.copy_keys", "source",
        Argument_Reader&& reader)
      {
        V_object obj;

        reader.start_overload();
        reader.required(obj);
        if(reader.end_overload())
          return (Value) std_array_copy_keys(obj);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("copy_values"),
      ASTERIA_BINDING(
        "std.array.copy_values", "source",
        Argument_Reader&& reader)
      {
        V_object obj;

        reader.start_overload();
        reader.required(obj);
        if(reader.end_overload())
          return (Value) std_array_copy_values(obj);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
