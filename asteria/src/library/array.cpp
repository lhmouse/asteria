// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "array.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/global_context.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

using Slice = pair<Aval::const_iterator, Aval::const_iterator>;

Slice do_slice(const Aval& data, Aval::const_iterator tbegin, const Iopt& length)
  {
    if(!length || (*length >= data.end() - tbegin)) {
      // Get the subrange from `tbegin` to the end.
      return ::std::make_pair(tbegin, data.end());
    }
    if(*length <= 0) {
      // Return an empty range.
      return ::std::make_pair(tbegin, tbegin);
    }
    // Don't go past the end.
    return ::std::make_pair(tbegin, tbegin + static_cast<ptrdiff_t>(*length));
  }

Slice do_slice(const Aval& data, const Ival& from, const Iopt& length)
  {
    auto slen = static_cast<int64_t>(data.size());
    if(from >= 0) {
      // Behave like `::std::string::substr()` except that no exception is thrown when `from` is
      // greater than `data.size()`.
      if(from >= slen) {
        return ::std::make_pair(data.end(), data.end());
      }
      return do_slice(data, data.begin() + static_cast<ptrdiff_t>(from), length);
    }
    // Wrap `from` from the end. Notice that `from + slen` will not overflow when `from` is negative
    // and `slen` is not.
    auto rfrom = from + slen;
    if(rfrom >= 0) {
      // Get a subrange from the wrapped index.
      return do_slice(data, data.begin() + static_cast<ptrdiff_t>(rfrom), length);
    }
    // Get a subrange from the beginning of `data`, if the wrapped index is before the first byte.
    if(!length) {
      // Get the subrange from the beginning to the end.
      return ::std::make_pair(data.begin(), data.end());
    }
    if(*length <= 0) {
      // Return an empty range.
      return ::std::make_pair(data.begin(), data.begin());
    }
    // Get a subrange excluding the part before the beginning.
    // Notice that `rfrom + *length` will not overflow when `rfrom` is negative and `*length` is not.
    return do_slice(data, data.begin(), rfrom + *length);
  }

template<typename IterT> opt<IterT> do_find_opt(IterT begin, IterT end, const Value& target)
  {
    for(auto it = ::std::move(begin);  it != end;  ++it) {
      // Compare the value using the builtin 3-way comparison operator.
      if(it->compare(target) == compare_equal)
        return it;
    }
    // Fail to find an element.
    return nullopt;
  }

Reference_root::S_temporary do_make_temporary(const Value& value)
  {
    Reference_root::S_temporary xref = { value };
    return xref;
  }

template<typename IterT> opt<IterT> do_find_if_opt(Global& global, IterT begin, IterT end,
                                                   const Fval& pred, bool match)
  {
    cow_vector<Reference> args;
    for(auto it = ::std::move(begin);  it != end;  ++it) {
      // Set up arguments for the user-defined predictor.
      args.resize(1, Reference_root::S_void());
      args.mut(0) = do_make_temporary(*it);
      // Call the predictor function and check the return value.
      auto self = pred.invoke(global, ::std::move(args));
      if(self.read().test() == match)
        return it;
    }
    // Fail to find an element.
    return nullopt;
  }

Compare do_compare(Global& global, cow_vector<Reference>& args,
                   const Fopt& kcomp, const Value& lhs, const Value& rhs)
  {
    if(ROCKET_EXPECT(!kcomp)) {
      // Use the builtin 3-way comparison operator.
      return lhs.compare(rhs);
    }
    // Set up arguments for the user-defined comparator.
    args.resize(2, Reference_root::S_void());
    args.mut(0) = do_make_temporary(lhs);
    args.mut(1) = do_make_temporary(rhs);
    // Call the predictor function and compare the result with `0`.
    auto self = kcomp.invoke(global, ::std::move(args));
    return self.read().compare(Ival(0));
  }

template<typename IterT>
    pair<IterT, bool> do_bsearch(Global& global, cow_vector<Reference>& args, IterT begin, IterT end,
                                 const Fopt& kcomp, const Value& target)
  {
    auto bpos = ::std::move(begin);
    auto epos = ::std::move(end);
    for(;;) {
      auto dist = epos - bpos;
      if(dist <= 0) {
        return { ::std::move(bpos), false };
      }
      auto mpos = bpos + dist / 2;
      // Compare `target` with the element in the middle.
      auto cmp = do_compare(global, args, kcomp, target, *mpos);
      if(cmp == compare_unordered) {
        ASTERIA_THROW("unordered elements (operands were `$1` and `$2`)", target, *mpos);
      }
      if(cmp == compare_equal) {
        return { ::std::move(mpos), true };
      }
      if(cmp == compare_less)
        epos = mpos;
      else
        bpos = mpos + 1;
    }
  }

template<typename IterT, typename PredT>
    IterT do_bound(Global& global, cow_vector<Reference>& args, IterT begin, IterT end,
                   const Fopt& kcomp, const Value& target, PredT&& pred)
  {
    auto bpos = ::std::move(begin);
    auto epos = ::std::move(end);
    for(;;) {
      auto dist = epos - bpos;
      if(dist <= 0) {
        return bpos;
      }
      auto mpos = bpos + dist / 2;
      // Compare `target` with the element in the middle.
      auto cmp = do_compare(global, args, kcomp, target, *mpos);
      if(cmp == compare_unordered) {
        ASTERIA_THROW("unordered elements (operands were `$1` and `$2`)", target, *mpos);
      }
      if(pred(cmp))
        epos = mpos;
      else
        bpos = mpos + 1;
    }
  }

Aval::iterator& do_merge_range(Aval::iterator& opos, Global& global, cow_vector<Reference>& args,
                               const Fopt& kcomp, Aval::iterator ibegin, Aval::iterator iend, bool unique)
  {
    for(auto ipos = ibegin;  ipos != iend;  ++ipos)
      if(!unique || (do_compare(global, args, kcomp, ipos[0], opos[-1]) != compare_equal))
        *(opos++) = ::std::move(*ipos);
    return opos;
  }

Aval::iterator do_merge_blocks(Aval& output, Global& global, cow_vector<Reference>& args,
                               const Fopt& kcomp, Aval&& input, ptrdiff_t bsize, bool unique)
  {
    ROCKET_ASSERT(output.size() >= input.size());
    // Define the range information for a pair of contiguous blocks.
    Aval::iterator bpos[2];
    Aval::iterator bend[2];
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
        if(cmp == compare_unordered) {
          ASTERIA_THROW("unordered elements (operands were `$1` and `$2`)", *(bpos[0]), *(bpos[1]));
        }
        // For Merge Sort to be stable, the two elements will only be swapped if the first one is greater
        // than the second one.
        bi = (cmp == compare_greater);
        // Move this element unless uniqueness is requested and it is equal to the previous output.
        bool discard = unique && (opos != output.begin())
                              && (do_compare(global, args, kcomp, *(bpos[bi]), opos[-1]) == compare_equal);
        if(!discard) {
          *(opos++) = ::std::move(*(bpos[bi]));
        }
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
        if(bpos[bi] == bend[bi]) {
          // `bi` is the index of the block that has been exhausted.
          break;
        }
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

Aval std_array_slice(Aval data, Ival from, Iopt length)
  {
    auto range = do_slice(data, from, length);
    if((range.first == data.begin()) && (range.second == data.end())) {
      // Use reference counting as our advantage.
      return data;
    }
    return Aval(range.first, range.second);
  }

Aval std_array_replace_slice(Aval data, Ival from, Aval replacement)
  {
    auto range = do_slice(data, from, nullopt);
    // Append segments.
    Aval res;
    res.reserve(data.size() - static_cast<size_t>(range.second - range.first) + replacement.size());
    res.append(data.begin(), range.first);
    res.append(replacement.begin(), replacement.end());
    res.append(range.second, data.end());
    return res;
  }

Aval std_array_replace_slice(Aval data, Ival from, Iopt length, Aval replacement)
  {
    auto range = do_slice(data, from, length);
    // Append segments.
    Aval res;
    res.reserve(data.size() - static_cast<size_t>(range.second - range.first) + replacement.size());
    res.append(data.begin(), range.first);
    res.append(replacement.begin(), replacement.end());
    res.append(range.second, data.end());
    return res;
  }

Iopt std_array_find(Aval data, Value target)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_opt(range.first, range.second, target);
    if(!qit) {
      return nullopt;
    }
    return *qit - data.begin();
  }

Iopt std_array_find(Aval data, Ival from, Value target)
  {
    auto range = do_slice(data, from, nullopt);
    auto qit = do_find_opt(range.first, range.second, target);
    if(!qit) {
      return nullopt;
    }
    return *qit - data.begin();
  }

Iopt std_array_find(Aval data, Ival from, Iopt length, Value target)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_opt(range.first, range.second, target);
    if(!qit) {
      return nullopt;
    }
    return *qit - data.begin();
  }

Iopt std_array_find_if(Global& global, Aval data, Fval predictor)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
    if(!qit) {
      return nullopt;
    }
    return *qit - data.begin();
  }

Iopt std_array_find_if(Global& global, Aval data, Ival from, Fval predictor)
  {
    auto range = do_slice(data, from, nullopt);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
    if(!qit) {
      return nullopt;
    }
    return *qit - data.begin();
  }

Iopt std_array_find_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
    if(!qit) {
      return nullopt;
    }
    return *qit - data.begin();
  }

Iopt std_array_find_if_not(Global& global, Aval data, Fval predictor)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
    if(!qit) {
      return nullopt;
    }
    return *qit - data.begin();
  }

Iopt std_array_find_if_not(Global& global, Aval data, Ival from, Fval predictor)
  {
    auto range = do_slice(data, from, nullopt);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
    if(!qit) {
      return nullopt;
    }
    return *qit - data.begin();
  }

Iopt std_array_find_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
    if(!qit) {
      return nullopt;
    }
    return *qit - data.begin();
  }

Iopt std_array_rfind(Aval data, Value target)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second),
                           ::std::make_reverse_iterator(range.first), target);
    if(!qit) {
      return nullopt;
    }
    return data.rend() - *qit - 1;
  }

Iopt std_array_rfind(Aval data, Ival from, Value target)
  {
    auto range = do_slice(data, from, nullopt);
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second),
                           ::std::make_reverse_iterator(range.first), target);
    if(!qit) {
      return nullopt;
    }
    return data.rend() - *qit - 1;
  }

Iopt std_array_rfind(Aval data, Ival from, Iopt length, Value target)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second),
                           ::std::make_reverse_iterator(range.first), target);
    if(!qit) {
      return nullopt;
    }
    return data.rend() - *qit - 1;
  }

Iopt std_array_rfind_if(Global& global, Aval data, Fval predictor)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second),
                                      ::std::make_reverse_iterator(range.first), predictor, true);
    if(!qit) {
      return nullopt;
    }
    return data.rend() - *qit - 1;
  }

Iopt std_array_rfind_if(Global& global, Aval data, Ival from, Fval predictor)
  {
    auto range = do_slice(data, from, nullopt);
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second),
                                      ::std::make_reverse_iterator(range.first), predictor, true);
    if(!qit) {
      return nullopt;
    }
    return data.rend() - *qit - 1;
  }

Iopt std_array_rfind_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second),
                                      ::std::make_reverse_iterator(range.first), predictor, true);
    if(!qit) {
      return nullopt;
    }
    return data.rend() - *qit - 1;
  }

Iopt std_array_rfind_if_not(Global& global, Aval data, Fval predictor)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second),
                                      ::std::make_reverse_iterator(range.first), predictor, false);
    if(!qit) {
      return nullopt;
    }
    return data.rend() - *qit - 1;
  }

Iopt std_array_rfind_if_not(Global& global, Aval data, Ival from, Fval predictor)
  {
    auto range = do_slice(data, from, nullopt);
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second),
                                      ::std::make_reverse_iterator(range.first), predictor, false);
    if(!qit) {
      return nullopt;
    }
    return data.rend() - *qit - 1;
  }

Iopt std_array_rfind_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second),
                                      ::std::make_reverse_iterator(range.first), predictor, false);
    if(!qit) {
      return nullopt;
    }
    return data.rend() - *qit - 1;
  }

Ival std_array_count(Aval data, Value target)
  {
    int64_t count = 0;
    auto range = ::std::make_pair(data.begin(), data.end());
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, target);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

Ival std_array_count(Aval data, Ival from, Value target)
  {
    int64_t count = 0;
    auto range = do_slice(data, from, nullopt);
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, target);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

Ival std_array_count(Aval data, Ival from, Iopt length, Value target)
  {
    int64_t count = 0;
    auto range = do_slice(data, from, length);
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, target);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

Ival std_array_count_if(Global& global, Aval data, Fval predictor)
  {
    int64_t count = 0;
    auto range = ::std::make_pair(data.begin(), data.end());
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

Ival std_array_count_if(Global& global, Aval data, Ival from, Fval predictor)
  {
    int64_t count = 0;
    auto range = do_slice(data, from, nullopt);
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

Ival std_array_count_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor)
  {
    int64_t count = 0;
    auto range = do_slice(data, from, length);
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

Ival std_array_count_if_not(Global& global, Aval data, Fval predictor)
  {
    int64_t count = 0;
    auto range = ::std::make_pair(data.begin(), data.end());
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

Ival std_array_count_if_not(Global& global, Aval data, Ival from, Fval predictor)
  {
    int64_t count = 0;
    auto range = do_slice(data, from, nullopt);
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

Ival std_array_count_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor)
  {
    int64_t count = 0;
    auto range = do_slice(data, from, length);
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::std::move(++*qit);
    }
    return count;
  }

Aval std_array_exclude(Aval data, Value target)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    ptrdiff_t dist = data.end() - range.second;
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, target);
      if(!qit) {
        break;
      }
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

Aval std_array_exclude(Aval data, Ival from, Value target)
  {
    auto range = do_slice(data, from, nullopt);
    ptrdiff_t dist = data.end() - range.second;
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, target);
      if(!qit) {
        break;
      }
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

Aval std_array_exclude(Aval data, Ival from, Iopt length, Value target)
  {
    auto range = do_slice(data, from, length);
    ptrdiff_t dist = data.end() - range.second;
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, target);
      if(!qit) {
        break;
      }
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

Aval std_array_exclude_if(Global& global, Aval data, Fval predictor)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    ptrdiff_t dist = data.end() - range.second;
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
      if(!qit) {
        break;
      }
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

Aval std_array_exclude_if(Global& global, Aval data, Ival from, Fval predictor)
  {
    auto range = do_slice(data, from, nullopt);
    ptrdiff_t dist = data.end() - range.second;
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
      if(!qit) {
        break;
      }
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

Aval std_array_exclude_if(Global& global, Aval data, Ival from, Iopt length, Fval predictor)
  {
    auto range = do_slice(data, from, length);
    ptrdiff_t dist = data.end() - range.second;
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
      if(!qit) {
        break;
      }
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

Aval std_array_exclude_if_not(Global& global, Aval data, Fval predictor)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    ptrdiff_t dist = data.end() - range.second;
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
      if(!qit) {
        break;
      }
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

Aval std_array_exclude_if_not(Global& global, Aval data, Ival from, Fval predictor)
  {
    auto range = do_slice(data, from, nullopt);
    ptrdiff_t dist = data.end() - range.second;
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
      if(!qit) {
        break;
      }
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

Aval std_array_exclude_if_not(Global& global, Aval data, Ival from, Iopt length, Fval predictor)
  {
    auto range = do_slice(data, from, length);
    ptrdiff_t dist = data.end() - range.second;
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
      if(!qit) {
        break;
      }
      range.first = data.erase(*qit);
      range.second = data.end() - dist;
    }
    return data;
  }

Bval std_array_is_sorted(Global& global, Aval data, Fopt comparator)
  {
    if(data.size() <= 1) {
      // If `data` contains no more than 2 elements, it is considered sorted.
      return true;
    }
    cow_vector<Reference> args;
    for(auto it = data.begin() + 1;  it != data.end();  ++it) {
      // Compare the two elements.
      auto cmp = do_compare(global, args, comparator, it[-1], it[0]);
      if((cmp == compare_greater) || (cmp == compare_unordered))
        return false;
    }
    return true;
  }

Iopt std_array_binary_search(Global& global, Aval data, Value target, Fopt comparator)
  {
    cow_vector<Reference> args;
    auto pair = do_bsearch(global, args, data.begin(), data.end(), comparator, target);
    if(!pair.second) {
      return nullopt;
    }
    return pair.first - data.begin();
  }

Ival std_array_lower_bound(Global& global, Aval data, Value target, Fopt comparator)
  {
    cow_vector<Reference> args;
    auto lpos = do_bound(global, args, data.begin(), data.end(), comparator, target,
                         [](Compare cmp) { return cmp != compare_greater;  });
    return lpos - data.begin();
  }

Ival std_array_upper_bound(Global& global, Aval data, Value target, Fopt comparator)
  {
    cow_vector<Reference> args;
    auto upos = do_bound(global, args, data.begin(), data.end(), comparator, target,
                         [](Compare cmp) { return cmp == compare_less;  });
    return upos - data.begin();
  }

pair<Ival, Ival> std_array_equal_range(Global& global, Aval data, Value target, Fopt comparator)
  {
    cow_vector<Reference> args;
    auto pair = do_bsearch(global, args, data.begin(), data.end(), comparator, target);
    auto lpos = do_bound(global, args, data.begin(), pair.first, comparator, target,
                         [](Compare cmp) { return cmp != compare_greater;  });
    auto upos = do_bound(global, args, pair.first, data.end(), comparator, target,
                         [](Compare cmp) { return cmp == compare_less;  });
    return ::std::make_pair(lpos - data.begin(), upos - lpos);
  }

Aval std_array_sort(Global& global, Aval data, Fopt comparator)
  {
    if(data.size() <= 1) {
      // Use reference counting as our advantage.
      return data;
    }
    // The Merge Sort algorithm requires `O(n)` space.
    Aval temp(data.size());
    // Merge blocks of exponential sizes.
    cow_vector<Reference> args;
    ptrdiff_t bsize = 1;
    while(bsize < data.ssize()) {
      do_merge_blocks(temp, global, args, comparator, ::std::move(data), bsize, false);
      data.swap(temp);
      bsize *= 2;
    }
    return data;
  }

Aval std_array_sortu(Global& global, Aval data, Fopt comparator)
  {
    if(data.size() <= 1) {
      // Use reference counting as our advantage.
      return data;
    }
    // The Merge Sort algorithm requires `O(n)` space.
    Aval temp(data.size());
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
    return data;
  }

Value std_array_max_of(Global& global, Aval data, Fopt comparator)
  {
    auto qmax = data.begin();
    if(qmax == data.end()) {
      // Return `null` if `data` is empty.
      return nullptr;
    }
    cow_vector<Reference> args;
    for(auto it = qmax + 1;  it != data.end();  ++it) {
      // Compare `*qmax` with the other elements, ignoring unordered elements.
      if(do_compare(global, args, comparator, *qmax, *it) != compare_less)
        continue;
      qmax = it;
    }
    return *qmax;
  }

Value std_array_min_of(Global& global, Aval data, Fopt comparator)
  {
    auto qmin = data.begin();
    if(qmin == data.end()) {
      // Return `null` if `data` is empty.
      return nullptr;
    }
    cow_vector<Reference> args;
    for(auto it = qmin + 1;  it != data.end();  ++it) {
      // Compare `*qmin` with the other elements, ignoring unordered elements.
      if(do_compare(global, args, comparator, *qmin, *it) != compare_greater)
        continue;
      qmin = it;
    }
    return *qmin;
  }

Aval std_array_reverse(Aval data)
  {
    // This is an easy matter, isn't it?
    return Aval(::std::make_move_iterator(data.rbegin()), ::std::make_move_iterator(data.rend()));
  }

Aval std_array_generate(Global& global, Fval generator, Ival length)
  {
    Aval data;
    data.reserve(static_cast<size_t>(length));
    cow_vector<Reference> args;
    for(int64_t i = 0;  i < length;  ++i) {
      // Set up arguments for the user-defined generator.
      args.resize(2, Reference_root::S_void());
      args.mut(0) = do_make_temporary(i);
      args.mut(1) = do_make_temporary(data.empty() ? null_value : data.back());
      // Call the generator function and push the return value.
      auto self = generator.invoke(global, ::std::move(args));
      data.emplace_back(self.read());
    }
    return data;
  }

Aval std_array_shuffle(Aval data, Iopt seed)
  {
    if(data.size() <= 1) {
      // Use reference counting as our advantage.
      return data;
    }
    // Create a linear congruential generator.
    uint64_t lcg = seed ? static_cast<uint64_t>(*seed) : generate_random_seed();
    // Shuffle elements.
    for(size_t i = 0;  i < data.size();  ++i) {
      // These arguments are the same as glibc's `drand48()` function.
      //   https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
      lcg *= 0x5DEECE66D;     // a
      lcg += 0xB;             // c
      lcg &= 0xFFFFFFFFFFFF;  // m
      // N.B. Conversion from an unsigned type to a floating-point type would result in performance penalty.
      // ratio <= [0.0, 1.0)
      double ratio = static_cast<double>(static_cast<int64_t>(lcg)) * 0x1p-48;
      // k <= [0, data.size())
      size_t k = static_cast<size_t>(static_cast<int64_t>(ratio * static_cast<double>(data.ssize())));
      if(k != i)
        swap(data.mut(k), data.mut(i));
    }
    return data;
  }

Aval std_array_rotate(Aval data, Ival shift)
  {
    if(data.size() <= 1) {
      // Use reference counting as our advantage.
      return data;
    }
    int64_t seek = shift % data.ssize();
    if(seek == 0) {
      // Use reference counting as our advantage.
      return data;
    }
    // Rotate it.
    seek = ((~seek >> 63) & data.ssize()) - seek;
    ::rocket::rotate(data.mut_data(), 0, static_cast<size_t>(seek), data.size());
    return data;
  }

Aval std_array_copy_keys(Oval source)
  {
    Aval data;
    data.reserve(source.size());
    ::rocket::for_each(source, [&](const auto& p) { data.emplace_back(Sval(p.first));  });
    return data;
  }

Aval std_array_copy_values(Oval source)
  {
    Aval data;
    data.reserve(source.size());
    ::rocket::for_each(source, [&](const auto& p) { data.emplace_back(p.second);  });
    return data;
  }

void create_bindings_array(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.array.slice()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("slice"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.slice"));
    // Parse arguments.
    Aval data;
    Ival from;
    Iopt length;
    if(reader.I().v(data).v(from).o(length).F()) {
      return std_array_slice(::std::move(data), ::std::move(from), ::std::move(length));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.slice(data, from, [length])`

  * Copies a subrange of `data` to create a new array. Elements are
    copied from `from` if it is non-negative, or from
    `lengthof(data) + from` otherwise. If `length` is set to an
    integer, no more than this number of elements will be copied.
    If it is absent, all elements from `from` to the end of `data`
    will be copied. If `from` is outside `data`, an empty array
    is returned.

  * Returns the specified subarray of `data`.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.replace_slice()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("replace_slice"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.replace"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Ival from;
    Aval replacement;
    if(reader.I().v(data).v(from).S(state).v(replacement).F()) {
      return std_array_replace_slice(::std::move(data), ::std::move(from), ::std::move(replacement));
    }
    Iopt length;
    if(reader.L(state).o(length).v(replacement).F()) {
      return std_array_replace_slice(::std::move(data), ::std::move(from), ::std::move(length),
                                     ::std::move(replacement));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.find()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.find"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Value target;
    if(reader.I().v(data).S(state).o(target).F()) {
      return std_array_find(::std::move(data), ::std::move(target));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).o(target).F()) {
      return std_array_find(::std::move(data), ::std::move(from), ::std::move(target));
    }
    Iopt length;
    if(reader.L(state).o(length).o(target).F()) {
      return std_array_find(::std::move(data), ::std::move(from), ::std::move(length),
                            ::std::move(target));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.find_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_if"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.find_if"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Fval predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      return std_array_find_if(global, ::std::move(data), ::std::move(predictor));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      return std_array_find_if(global, ::std::move(data), ::std::move(from), ::std::move(predictor));
    }
    Iopt length;
    if(reader.L(state).o(length).v(predictor).F()) {
      return std_array_find_if(global, ::std::move(data), ::std::move(from), ::std::move(length),
                                       ::std::move(predictor));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.find_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_if_not"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.find_if_not"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Fval predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      return std_array_find_if_not(global, ::std::move(data), ::std::move(predictor));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      return std_array_find_if_not(global, ::std::move(data), ::std::move(from),
                                           ::std::move(predictor));
    }
    Iopt length;
    if(reader.L(state).o(length).v(predictor).F()) {
      return std_array_find_if_not(global, ::std::move(data), ::std::move(from),
                                           ::std::move(length), ::std::move(predictor));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.rfind()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.rfind"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Value target;
    if(reader.I().v(data).S(state).o(target).F()) {
      return std_array_rfind(::std::move(data), ::std::move(target));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).o(target).F()) {
      return std_array_rfind(::std::move(data), ::std::move(from), ::std::move(target));
    }
    Iopt length;
    if(reader.L(state).o(length).o(target).F()) {
      return std_array_rfind(::std::move(data), ::std::move(from), ::std::move(length),
                             ::std::move(target));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.rfind_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind_if"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.rfind_if"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Fval predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      return std_array_rfind_if(global, ::std::move(data), ::std::move(predictor));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      return std_array_rfind_if(global, ::std::move(data), ::std::move(from),
                                        ::std::move(predictor));
    }
    Iopt length;
    if(reader.L(state).o(length).v(predictor).F()) {
      return std_array_rfind_if(global, ::std::move(data), ::std::move(from),
                                        ::std::move(length), ::std::move(predictor));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.rfind_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind_if_not"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.rfind_if_not"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Fval predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      return std_array_rfind_if_not(global, ::std::move(data), ::std::move(predictor));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      return std_array_rfind_if_not(global, ::std::move(data), ::std::move(from),
                                            ::std::move(predictor));
    }
    Iopt length;
    if(reader.L(state).o(length).v(predictor).F()) {
      return std_array_rfind_if_not(global, ::std::move(data), ::std::move(from), ::std::move(length),
                                            ::std::move(predictor));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.count()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("count"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.count"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Value target;
    if(reader.I().v(data).S(state).o(target).F()) {
      return std_array_count(::std::move(data), ::std::move(target));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).o(target).F()) {
      return std_array_count(::std::move(data), ::std::move(from), ::std::move(target));
    }
    Iopt length;
    if(reader.L(state).o(length).o(target).F()) {
      return std_array_count(::std::move(data), ::std::move(from), ::std::move(length),
                             ::std::move(target));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.count_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("count_if"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.count_if"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Fval predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      return std_array_count_if(global, ::std::move(data), ::std::move(predictor));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      return std_array_count_if(global, ::std::move(data),  ::std::move(from),
                                        ::std::move(predictor));
    }
    Iopt length;
    if(reader.L(state).o(length).v(predictor).F()) {
      return std_array_count_if(global, ::std::move(data), ::std::move(from), ::std::move(length),
                                        ::std::move(predictor));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.count_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("count_if_not"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.count_if_not"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Fval predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      return std_array_count_if_not(global, ::std::move(data), ::std::move(predictor));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      return std_array_count_if_not(global, ::std::move(data), ::std::move(from),
                                            ::std::move(predictor));
    }
    Iopt length;
    if(reader.L(state).o(length).v(predictor).F()) {
      return std_array_count_if_not(global, ::std::move(data), ::std::move(from), ::std::move(length),
                                            ::std::move(predictor));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.exclude()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("exclude"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.exclude"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Value target;
    if(reader.I().v(data).S(state).o(target).F()) {
      return std_array_exclude(::std::move(data), ::std::move(target));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).o(target).F()) {
      return std_array_exclude(::std::move(data), ::std::move(from), ::std::move(target));
    }
    Iopt length;
    if(reader.L(state).o(length).o(target).F()) {
      return std_array_exclude(::std::move(data), ::std::move(from), ::std::move(length),
                               ::std::move(target));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.exclude_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("exclude_if"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.exclude_if"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Fval predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      return std_array_exclude_if(global, ::std::move(data), ::std::move(predictor));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      return std_array_exclude_if(global, ::std::move(data),  ::std::move(from),
                                          ::std::move(predictor));
    }
    Iopt length;
    if(reader.L(state).o(length).v(predictor).F()) {
      return std_array_exclude_if(global, ::std::move(data), ::std::move(from), ::std::move(length),
                                          ::std::move(predictor));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.exclude_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("exclude_if_not"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.exclude_if_not"));
    Argument_Reader::State state;
    // Parse arguments.
    Aval data;
    Fval predictor;
    if(reader.I().v(data).S(state).v(predictor).F()) {
      return std_array_exclude_if_not(global, ::std::move(data), ::std::move(predictor));
    }
    Ival from;
    if(reader.L(state).v(from).S(state).v(predictor).F()) {
      return std_array_exclude_if_not(global, ::std::move(data), ::std::move(from),
                                              ::std::move(predictor));
    }
    Iopt length;
    if(reader.L(state).o(length).v(predictor).F()) {
      return std_array_exclude_if_not(global, ::std::move(data), ::std::move(from), ::std::move(length),
                                              ::std::move(predictor));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.is_sorted()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("is_sorted"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.is_sorted"));
    // Parse arguments.
    Aval data;
    Fopt comparator;
    if(reader.I().v(data).o(comparator).F()) {
      return std_array_is_sorted(global, ::std::move(data), ::std::move(comparator));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.binary_search()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("binary_search"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.binary_search"));
    // Parse arguments.
    Aval data;
    Value target;
    Fopt comparator;
    if(reader.I().v(data).o(target).o(comparator).F()) {
      return std_array_binary_search(global, ::std::move(data), ::std::move(target),
                                             ::std::move(comparator));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.lower_bound()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("lower_bound"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.lower_bound"));
    // Parse arguments.
    Aval data;
    Value target;
    Fopt comparator;
    if(reader.I().v(data).o(target).o(comparator).F()) {
      return std_array_lower_bound(global, ::std::move(data), ::std::move(target),
                                           ::std::move(comparator));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.lower_bound(data, target, [comparator])`

  * Finds the first element in `data` that is greater than or equal
    to `target` and precedes all elements that are less than
    `target` if any. The principle of user-defined `comparator`s is
    the same as the `is_sorted()` function. As a consequence, the
    function call `is_sorted(data, comparator)` shall yield `true`
    prior to this call, otherwise the effect is undefined.

  * Returns the subscript of such an element as an integer. This
    function returns `lengthof(data)` if all elements are less than
    `target`.

  * Throws an exception if `data` has not been sorted properly. Be
    advised that in this case there is no guarantee whether an
    exception will be thrown or not.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.upper_bound()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("upper_bound"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.upper_bound"));
    // Parse arguments.
    Aval data;
    Value target;
    Fopt comparator;
    if(reader.I().v(data).o(target).o(comparator).F()) {
      return std_array_upper_bound(global, ::std::move(data), ::std::move(target),
                                           ::std::move(comparator));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.upper_bound(data, target, [comparator])`

  * Finds the first element in `data` that is greater than `target`
    and precedes all elements that are less than or equal to
    `target` if any. The principle of user-defined `comparator`s is
    the same as the `is_sorted()` function. As a consequence, the
    function call `is_sorted(data, comparator)` shall yield `true`
    prior to this call, otherwise the effect is undefined.

  * Returns the subscript of such an element as an integer. This
    function returns `lengthof(data)` if all elements are less than
    or equal to `target`.

  * Throws an exception if `data` has not been sorted properly. Be
    advised that in this case there is no guarantee whether an
    exception will be thrown or not.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.equal_range()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("equal_range"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.lower_bound"));
    // Parse arguments.
    Aval data;
    Value target;
    Fopt comparator;
    if(reader.I().v(data).o(target).o(comparator).F()) {
      auto pair = std_array_equal_range(global, ::std::move(data), ::std::move(target),
                                                ::std::move(comparator));
      // This function returns a `pair`, but we would like to return an array so convert it.
      Aval rval(2);
      rval.mut(0) = ::std::move(pair.first);
      rval.mut(1) = ::std::move(pair.second);
      return rval;
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.sort()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sort"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.sort"));
    // Parse arguments.
    Aval data;
    Fopt comparator;
    if(reader.I().v(data).o(comparator).F()) {
      return std_array_sort(global, ::std::move(data), ::std::move(comparator));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.sortu()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sortu"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.sortu"));
    // Parse arguments.
    Aval data;
    Fopt comparator;
    if(reader.I().v(data).o(comparator).F()) {
      return std_array_sortu(global, ::std::move(data), ::std::move(comparator));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
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
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.max_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("max_of"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.max_of"));
    // Parse arguments.
    Aval data;
    Fopt comparator;
    if(reader.I().v(data).o(comparator).F()) {
      return std_array_max_of(global, ::std::move(data), ::std::move(comparator));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.max_of(data, [comparator])`

  * Finds the maximum element in `data`. The principle of
    user-defined `comparator`s is the same as the `is_sorted()`
    function. Elements that are unordered with the first element
    are ignored silently.

  * Returns a copy of the maximum element, or `null` if `data` is
    empty.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.min_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("min_of"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.min_of"));
    // Parse arguments.
    Aval data;
    Fopt comparator;
    if(reader.I().v(data).o(comparator).F()) {
      return std_array_min_of(global, ::std::move(data), ::std::move(comparator));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.min_of(data, [comparator])`

  * Finds the minimum element in `data`. The principle of
    user-defined `comparator`s is the same as the `is_sorted()`
    function. Elements that are unordered with the first element
    are ignored silently.

  * Returns a copy of the minimum element, or `null` if `data` is
    empty.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.reverse()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("reverse"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.reverse"));
    // Parse arguments.
    Aval data;
    Fopt comparator;
    if(reader.I().v(data).o(comparator).F()) {
      return std_array_reverse(::std::move(data));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.reverse(data)`

  * Reverses an array. This function returns a new array without
    modifying `data`.

  * Returns the reversed array.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.generate()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("generate"),
      Fval(
[](cow_vector<Reference>&& args, Reference&& /*self*/, Global& global) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.generate"));
    // Parse arguments.
    Fval generator;
    Ival length;
    if(reader.I().v(generator).v(length).F()) {
      return std_array_generate(global, ::std::move(generator), ::std::move(length));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.generate(generator, length)`

  * Calls `generator` repeatedly up to `length` times and returns
    an array consisting of all values returned. `generator` shall
    be a binary function. The first argument will be the number of
    elements having been generated; the second argument is the
    previous element generated, or `null` in the case of the first
    element.

  * Returns an array containing all values generated.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.shuffle()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("shuffle"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.shuffle"));
    // Parse arguments.
    Aval data;
    Iopt seed;
    if(reader.I().v(data).o(seed).F()) {
      return std_array_shuffle(::std::move(data), ::std::move(seed));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.shuffle(data, [seed])`

  * Shuffles elements in `data` randomly. If `seed` is set to an
    integer, the internal pseudo random number generator will be
    initialized with it and will produce the same series of numbers
    for a specific `seed` value. If it is absent, an unspecified
    seed is generated when this function is called. This function
    returns a new array without modifying `data`.

  * Returns the shuffled array.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.rotate()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rotate"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.rotate"));
    // Parse arguments.
    Aval data;
    Ival shift;
    if(reader.I().v(data).v(shift).F()) {
      return std_array_rotate(::std::move(data), ::std::move(shift));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.rotate(data, shift)`

  * Rotates elements in `data` by `shift`. That is, unless `data`
    is empty, the element at subscript `x` is moved to subscript
    `(x + shift) % lengthof(data)`. No element is added or removed.

  * Returns the rotated array. If `data` is empty, an empty array
    is returned.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.copy_keys()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("copy_keys"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.copy_keys"));
    // Parse arguments.
    Oval source;
    if(reader.I().v(source).F()) {
      return std_array_copy_keys(::std::move(source));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.copy_keys(source)`

  * Copies all keys from `source`, which shall be an object, to
    create an array.

  * Returns an array of all keys in `source`.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // `std.array.copy_values()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("copy_values"),
      Fval(
[](cow_vector<Reference>&& args) -> Value
  {
    Argument_Reader reader(::rocket::ref(args), ::rocket::sref("std.array.copy_values"));
    // Parse arguments.
    Oval source;
    if(reader.I().v(source).F()) {
      return std_array_copy_values(::std::move(source));
    }
    // Fail.
    reader.throw_no_matching_function_call();
  },
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.array.copy_values(source)`

  * Copies all values from `source`, which shall be an object, to
    create an array.

  * Returns an array of all values in `source`.
)'''''''''''''''"  """"""""""""""""""""""""""""""""""""""""""""""""
      ));
    //===================================================================
    // End of `std.array`
    //===================================================================
  }

}  // namespace Asteria
