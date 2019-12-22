// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_array.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../runtime/global_context.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

pair<G_array::const_iterator, G_array::const_iterator> do_slice(const G_array& text, G_array::const_iterator tbegin, const opt<G_integer>& length)
  {
    if(!length || (*length >= text.end() - tbegin)) {
      // Get the subrange from `tbegin` to the end.
      return ::std::make_pair(tbegin, text.end());
    }
    if(*length <= 0) {
      // Return an empty range.
      return ::std::make_pair(tbegin, tbegin);
    }
    // Don't go past the end.
    return ::std::make_pair(tbegin, tbegin + static_cast<ptrdiff_t>(*length));
  }
pair<G_array::const_iterator, G_array::const_iterator> do_slice(const G_array& text, const G_integer& from, const opt<G_integer>& length)
  {
    auto slen = static_cast<int64_t>(text.size());
    if(from >= 0) {
      // Behave like `::std::string::substr()` except that no exception is thrown when `from` is greater than `text.size()`.
      if(from >= slen) {
        return ::std::make_pair(text.end(), text.end());
      }
      return do_slice(text, text.begin() + static_cast<ptrdiff_t>(from), length);
    }
    // Wrap `from` from the end. Notice that `from + slen` will not overflow when `from` is negative and `slen` is not.
    auto rfrom = from + slen;
    if(rfrom >= 0) {
      // Get a subrange from the wrapped index.
      return do_slice(text, text.begin() + static_cast<ptrdiff_t>(rfrom), length);
    }
    // Get a subrange from the beginning of `text`, if the wrapped index is before the first byte.
    if(!length) {
      // Get the subrange from the beginning to the end.
      return ::std::make_pair(text.begin(), text.end());
    }
    if(*length <= 0) {
      // Return an empty range.
      return ::std::make_pair(text.begin(), text.begin());
    }
    // Get a subrange excluding the part before the beginning.
    // Notice that `rfrom + *length` will not overflow when `rfrom` is negative and `*length` is not.
    return do_slice(text, text.begin(), rfrom + *length);
  }

template<typename IteratorT> opt<IteratorT> do_find_opt(IteratorT begin, IteratorT end, const Value& target)
  {
    for(auto it = ::rocket::move(begin); it != end; ++it) {
      // Compare the value using the builtin 3-way comparison operator.
      if(it->compare(target) == compare_equal)
        return ::rocket::move(it);
    }
    // Fail to find an element.
    return ::rocket::clear;
  }

inline void do_push_argument(cow_vector<Reference>& args, const Value& value)
  {
    Reference_Root::S_temporary xref = { value };
    args.emplace_back(::rocket::move(xref));
  }

template<typename IteratorT> opt<IteratorT> do_find_if_opt(const Global_Context& global, IteratorT begin, IteratorT end,
                                                           const G_function& predictor, bool match)
  {
    for(auto it = ::rocket::move(begin); it != end; ++it) {
      // Set up arguments for the user-defined predictor.
      cow_vector<Reference> args;
      do_push_argument(args, *it);
      // Call the predictor function and check the return value.
      Reference self;
      predictor->invoke(self, global, ::rocket::move(args));
      self.finish_call(global);
      if(self.read().test() == match)
        return ::rocket::move(it);
    }
    // Fail to find an element.
    return ::rocket::clear;
  }

Compare do_compare(const Global_Context& global, const G_function& comp, const Value& lhs, const Value& rhs)
  {
    // Set up arguments for the user-defined comparator.
    cow_vector<Reference> args;
    do_push_argument(args, lhs);
    do_push_argument(args, rhs);
    // Call the predictor function and compare the result with `0`.
    Reference self;
    comp->invoke(self, global, ::rocket::move(args));
    self.finish_call(global);
    return self.read().compare(G_integer(0));
  }

Compare do_compare(const Global_Context& global, const opt<G_function>& comp, const Value& lhs, const Value& rhs)
  {
    return ROCKET_UNEXPECT(comp) ? do_compare(global, *comp, lhs, rhs) : lhs.compare(rhs);
  }

template<typename IteratorT>
    pair<IteratorT, bool> do_bsearch(const Global_Context& global, IteratorT begin, IteratorT end,
                                     const opt<G_function>& comparator, const Value& target)
  {
    auto bpos = ::rocket::move(begin);
    auto epos = ::rocket::move(end);
    do {
      auto dist = epos - bpos;
      if(dist <= 0) {
        return ::std::make_pair(::rocket::move(bpos), false);
      }
      auto mpos = bpos + dist / 2;
      // Compare `target` with the element in the middle.
      auto cmp = do_compare(global, comparator, target, *mpos);
      if(cmp == compare_unordered) {
        ASTERIA_THROW("unordered elements (operands were `$1` and `$2`)", target, *mpos);
      }
      if(cmp == compare_equal) {
        return ::std::make_pair(::rocket::move(mpos), true);
      }
      if(cmp == compare_less)
        epos = mpos;
      else
        bpos = mpos + 1;
    } while(true);
  }

template<typename IteratorT, typename PredT>
    IteratorT do_bound(const Global_Context& global, IteratorT begin, IteratorT end,
                       const opt<G_function>& comparator, const Value& target, PredT&& pred)
  {
    auto bpos = ::rocket::move(begin);
    auto epos = ::rocket::move(end);
    do {
      auto dist = epos - bpos;
      if(dist <= 0) {
        return ::rocket::move(bpos);
      }
      auto mpos = bpos + dist / 2;
      // Compare `target` with the element in the middle.
      auto cmp = do_compare(global, comparator, target, *mpos);
      if(cmp == compare_unordered) {
        ASTERIA_THROW("unordered elements (operands were `$1` and `$2`)", target, *mpos);
      }
      if(::rocket::forward<PredT>(pred)(cmp))
        epos = mpos;
      else
        bpos = mpos + 1;
    } while(true);
  }

void do_unique_move(G_array::iterator& opos, const Global_Context& global, const opt<G_function>& comparator,
                    G_array::iterator ibegin, G_array::iterator iend, bool unique)
  {
    for(auto ipos = ibegin; ipos != iend; ++ipos) {
      if(unique && (do_compare(global, comparator, *ipos, opos[-1]) == compare_equal)) {
        continue;
      }
      *(opos++) = ::rocket::move(*ipos);
    }
  }

G_array::iterator do_merge_blocks(G_array& output, const Global_Context& global, const opt<G_function>& comparator,
                                  G_array&& input, ptrdiff_t bsize, bool unique)
  {
    ROCKET_ASSERT(output.size() >= input.size());
    // Define the range information for a pair of contiguous blocks.
    G_array::iterator bpos[2];
    G_array::iterator bend[2];
    // Merge adjacent blocks of `bsize` elements.
    auto opos = output.mut_begin();
    auto ipos = input.mut_begin();
    auto iend = input.mut_end();
    for(;;) {
      // Get the block of the first block to merge.
      bpos[0] = ipos;
      if(iend - ipos <= bsize) {
        // Copy all remaining elements.
        ROCKET_ASSERT(opos != output.begin());
        do_unique_move(opos, global, comparator, ipos, iend, unique);
        break;
      }
      ipos += bsize;
      bend[0] = ipos;
      // Get the range of the second block to merge.
      bpos[1] = ipos;
      ipos += ::rocket::min(iend - ipos, bsize);
      bend[1] = ipos;
      // Merge elements one by one, until either block has been exhausted, then store the index of it here.
      size_t bi;
      for(;;) {
        auto cmp = do_compare(global, comparator, *(bpos[0]), *(bpos[1]));
        if(cmp == compare_unordered) {
          ASTERIA_THROW("unordered elements (operands were `$1` and `$2`)", *(bpos[0]), *(bpos[1]));
        }
        // For Merge Sort to be stable, the two elements will only be swapped if the first one is greater than the second one.
        bi = (cmp == compare_greater);
        // Move this element unless uniqueness is requested and it is equal to the previous output.
        if(!(unique && (opos != output.begin()) && (do_compare(global, comparator, *(bpos[bi]), opos[-1]) == compare_equal))) {
          *(opos++) = ::rocket::move(*(bpos[bi]));
        }
        bpos[bi]++;
        // When uniqueness is requested, if elements from the two blocks are equal, discard the one from the second block.
        // This may exhaust the second block.
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
      do_unique_move(opos, global, comparator, bpos[bi], bend[bi], unique);
    }
    return opos;
  }

}  // namespace

G_array std_array_slice(const G_array& data, const G_integer& from, const opt<G_integer>& length)
  {
    auto range = do_slice(data, from, length);
    if((range.first == data.begin()) && (range.second == data.end())) {
      // Use reference counting as our advantage.
      return data;
    }
    return G_array(range.first, range.second);
  }

G_array std_array_replace_slice(const G_array& data, const G_integer& from, const G_array& replacement)
  {
    auto range = do_slice(data, from, ::rocket::clear);
    // Append segments.
    G_array res;
    res.reserve(data.size() - static_cast<size_t>(range.second - range.first) + replacement.size());
    res.append(data.begin(), range.first);
    res.append(replacement.begin(), replacement.end());
    res.append(range.second, data.end());
    return res;
  }

G_array std_array_replace_slice(const G_array& data, const G_integer& from, const opt<G_integer>& length, const G_array& replacement)
  {
    auto range = do_slice(data, from, length);
    // Append segments.
    G_array res;
    res.reserve(data.size() - static_cast<size_t>(range.second - range.first) + replacement.size());
    res.append(data.begin(), range.first);
    res.append(replacement.begin(), replacement.end());
    res.append(range.second, data.end());
    return res;
  }

opt<G_integer> std_array_find(const G_array& data, const Value& target)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_opt(range.first, range.second, target);
    if(!qit) {
      return ::rocket::clear;
    }
    return *qit - data.begin();
  }

opt<G_integer> std_array_find(const G_array& data, const G_integer& from, const Value& target)
  {
    auto range = do_slice(data, from, ::rocket::clear);
    auto qit = do_find_opt(range.first, range.second, target);
    if(!qit) {
      return ::rocket::clear;
    }
    return *qit - data.begin();
  }

opt<G_integer> std_array_find(const G_array& data, const G_integer& from, const opt<G_integer>& length, const Value& target)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_opt(range.first, range.second, target);
    if(!qit) {
      return ::rocket::clear;
    }
    return *qit - data.begin();
  }

opt<G_integer> std_array_find_if(const Global_Context& global, const G_array& data, const G_function& predictor)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
    if(!qit) {
      return ::rocket::clear;
    }
    return *qit - data.begin();
  }

opt<G_integer> std_array_find_if(const Global_Context& global, const G_array& data, const G_integer& from, const G_function& predictor)
  {
    auto range = do_slice(data, from, ::rocket::clear);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
    if(!qit) {
      return ::rocket::clear;
    }
    return *qit - data.begin();
  }

opt<G_integer> std_array_find_if(const Global_Context& global, const G_array& data, const G_integer& from, const opt<G_integer>& length, const G_function& predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
    if(!qit) {
      return ::rocket::clear;
    }
    return *qit - data.begin();
  }

opt<G_integer> std_array_find_if_not(const Global_Context& global, const G_array& data, const G_function& predictor)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
    if(!qit) {
      return ::rocket::clear;
    }
    return *qit - data.begin();
  }

opt<G_integer> std_array_find_if_not(const Global_Context& global, const G_array& data, const G_integer& from, const G_function& predictor)
  {
    auto range = do_slice(data, from, ::rocket::clear);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
    if(!qit) {
      return ::rocket::clear;
    }
    return *qit - data.begin();
  }

opt<G_integer> std_array_find_if_not(const Global_Context& global, const G_array& data, const G_integer& from, const opt<G_integer>& length, const G_function& predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
    if(!qit) {
      return ::rocket::clear;
    }
    return *qit - data.begin();
  }

opt<G_integer> std_array_rfind(const G_array& data, const Value& target)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first), target);
    if(!qit) {
      return ::rocket::clear;
    }
    return data.rend() - *qit - 1;
  }

opt<G_integer> std_array_rfind(const G_array& data, const G_integer& from, const Value& target)
  {
    auto range = do_slice(data, from, ::rocket::clear);
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first), target);
    if(!qit) {
      return ::rocket::clear;
    }
    return data.rend() - *qit - 1;
  }

opt<G_integer> std_array_rfind(const G_array& data, const G_integer& from, const opt<G_integer>& length, const Value& target)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first), target);
    if(!qit) {
      return ::rocket::clear;
    }
    return data.rend() - *qit - 1;
  }

opt<G_integer> std_array_rfind_if(const Global_Context& global, const G_array& data, const G_function& predictor)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first), predictor, true);
    if(!qit) {
      return ::rocket::clear;
    }
    return data.rend() - *qit - 1;
  }

opt<G_integer> std_array_rfind_if(const Global_Context& global, const G_array& data, const G_integer& from, const G_function& predictor)
  {
    auto range = do_slice(data, from, ::rocket::clear);
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first), predictor, true);
    if(!qit) {
      return ::rocket::clear;
    }
    return data.rend() - *qit - 1;
  }

opt<G_integer> std_array_rfind_if(const Global_Context& global, const G_array& data, const G_integer& from, const opt<G_integer>& length, const G_function& predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first), predictor, true);
    if(!qit) {
      return ::rocket::clear;
    }
    return data.rend() - *qit - 1;
  }

opt<G_integer> std_array_rfind_if_not(const Global_Context& global, const G_array& data, const G_function& predictor)
  {
    auto range = ::std::make_pair(data.begin(), data.end());
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first), predictor, false);
    if(!qit) {
      return ::rocket::clear;
    }
    return data.rend() - *qit - 1;
  }

opt<G_integer> std_array_rfind_if_not(const Global_Context& global, const G_array& data, const G_integer& from, const G_function& predictor)
  {
    auto range = do_slice(data, from, ::rocket::clear);
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first), predictor, false);
    if(!qit) {
      return ::rocket::clear;
    }
    return data.rend() - *qit - 1;
  }

opt<G_integer> std_array_rfind_if_not(const Global_Context& global, const G_array& data, const G_integer& from, const opt<G_integer>& length, const G_function& predictor)
  {
    auto range = do_slice(data, from, length);
    auto qit = do_find_if_opt(global, ::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first), predictor, false);
    if(!qit) {
      return ::rocket::clear;
    }
    return data.rend() - *qit - 1;
  }

G_integer std_array_count(const G_array& data, const Value& target)
  {
    G_integer count = 0;
    auto range = ::std::make_pair(data.begin(), data.end());
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, target);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::rocket::move(++*qit);
    }
    return count;
  }

G_integer std_array_count(const G_array& data, const G_integer& from, const Value& target)
  {
    G_integer count = 0;
    auto range = do_slice(data, from, ::rocket::clear);
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, target);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::rocket::move(++*qit);
    }
    return count;
  }

G_integer std_array_count(const G_array& data, const G_integer& from, const opt<G_integer>& length, const Value& target)
  {
    G_integer count = 0;
    auto range = do_slice(data, from, length);
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, target);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::rocket::move(++*qit);
    }
    return count;
  }

G_integer std_array_count_if(const Global_Context& global, const G_array& data, const G_function& predictor)
  {
    G_integer count = 0;
    auto range = ::std::make_pair(data.begin(), data.end());
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::rocket::move(++*qit);
    }
    return count;
  }

G_integer std_array_count_if(const Global_Context& global, const G_array& data, const G_integer& from, const G_function& predictor)
  {
    G_integer count = 0;
    auto range = do_slice(data, from, ::rocket::clear);
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::rocket::move(++*qit);
    }
    return count;
  }

G_integer std_array_count_if(const Global_Context& global, const G_array& data, const G_integer& from, const opt<G_integer>& length, const G_function& predictor)
  {
    G_integer count = 0;
    auto range = do_slice(data, from, length);
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, true);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::rocket::move(++*qit);
    }
    return count;
  }

G_integer std_array_count_if_not(const Global_Context& global, const G_array& data, const G_function& predictor)
  {
    G_integer count = 0;
    auto range = ::std::make_pair(data.begin(), data.end());
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::rocket::move(++*qit);
    }
    return count;
  }

G_integer std_array_count_if_not(const Global_Context& global, const G_array& data, const G_integer& from, const G_function& predictor)
  {
    G_integer count = 0;
    auto range = do_slice(data, from, ::rocket::clear);
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::rocket::move(++*qit);
    }
    return count;
  }

G_integer std_array_count_if_not(const Global_Context& global, const G_array& data, const G_integer& from, const opt<G_integer>& length, const G_function& predictor)
  {
    G_integer count = 0;
    auto range = do_slice(data, from, length);
    for(;;) {
      auto qit = do_find_if_opt(global, range.first, range.second, predictor, false);
      if(!qit) {
        break;
      }
      ++count;
      range.first = ::rocket::move(++*qit);
    }
    return count;
  }

G_boolean std_array_is_sorted(const Global_Context& global, const G_array& data, const opt<G_function>& comparator)
  {
    if(data.size() <= 1) {
      // If `data` contains no more than 2 elements, it is considered sorted.
      return true;
    }
    for(auto it = data.begin() + 1; it != data.end(); ++it) {
      // Compare the two elements.
      auto cmp = do_compare(global, comparator, it[-1], it[0]);
      if((cmp == compare_greater) || (cmp == compare_unordered))
        return false;
    }
    return true;
  }

opt<G_integer> std_array_binary_search(const Global_Context& global, const G_array& data, const Value& target, const opt<G_function>& comparator)
  {
    auto pair = do_bsearch(global, data.begin(), data.end(), comparator, target);
    if(!pair.second) {
      return ::rocket::clear;
    }
    return pair.first - data.begin();
  }

G_integer std_array_lower_bound(const Global_Context& global, const G_array& data, const Value& target, const opt<G_function>& comparator)
  {
    auto lpos = do_bound(global, data.begin(), data.end(), comparator, target, [](Compare cmp) { return cmp != compare_greater;  });
    return lpos - data.begin();
  }

G_integer std_array_upper_bound(const Global_Context& global, const G_array& data, const Value& target, const opt<G_function>& comparator)
  {
    auto upos = do_bound(global, data.begin(), data.end(), comparator, target, [](Compare cmp) { return cmp == compare_less;  });
    return upos - data.begin();
  }

pair<G_integer, G_integer> std_array_equal_range(const Global_Context& global, const G_array& data, const Value& target, const opt<G_function>& comparator)
  {
    auto pair = do_bsearch(global, data.begin(), data.end(), comparator, target);
    auto lpos = do_bound(global, data.begin(), pair.first, comparator, target, [](Compare cmp) { return cmp != compare_greater;  });
    auto upos = do_bound(global, pair.first, data.end(), comparator, target, [](Compare cmp) { return cmp == compare_less;  });
    return { lpos - data.begin(), upos - data.begin() };
  }

G_array std_array_sort(const Global_Context& global, const G_array& data, const opt<G_function>& comparator)
  {
    if(data.size() <= 1) {
      // Use reference counting as our advantage.
      return data;
    }
    // The Merge Sort algorithm requires `O(n)` space.
    G_array res = data;
    G_array temp(data.size());
    // Merge blocks of exponential sizes.
    ptrdiff_t bsize = 1;
    while(bsize < res.ssize()) {
      do_merge_blocks(temp, global, comparator, ::rocket::move(res), bsize, false);
      res.swap(temp);
      bsize *= 2;
    }
    return res;
  }

G_array std_array_sortu(const Global_Context& global, const G_array& data, const opt<G_function>& comparator)
  {
    if(data.size() <= 1) {
      // Use reference counting as our advantage.
      return data;
    }
    // The Merge Sort algorithm requires `O(n)` space.
    G_array res = data;
    G_array temp(res.size());
    // Merge blocks of exponential sizes.
    ptrdiff_t bsize = 1;
    while(bsize * 2 < res.ssize()) {
      do_merge_blocks(temp, global, comparator, ::rocket::move(res), bsize, false);
      res.swap(temp);
      bsize *= 2;
    }
    auto epos = do_merge_blocks(temp, global, comparator, ::rocket::move(res), bsize, true);
    temp.erase(epos, temp.end());
    res.swap(temp);
    return res;
  }

Value std_array_max_of(const Global_Context& global, const G_array& data, const opt<G_function>& comparator)
  {
    auto qmax = data.begin();
    if(qmax == data.end()) {
      // Return `null` if `data` is empty.
      return nullptr;
    }
    for(auto it = qmax + 1; it != data.end(); ++it) {
      // Compare `*qmax` with the other elements, ignoring unordered elements.
      if(do_compare(global, comparator, *qmax, *it) != compare_less) {
        continue;
      }
      qmax = it;
    }
    return *qmax;
  }

Value std_array_min_of(const Global_Context& global, const G_array& data, const opt<G_function>& comparator)
  {
    auto qmin = data.begin();
    if(qmin == data.end()) {
      // Return `null` if `data` is empty.
      return nullptr;
    }
    for(auto it = qmin + 1; it != data.end(); ++it) {
      // Compare `*qmin` with the other elements, ignoring unordered elements.
      if(do_compare(global, comparator, *qmin, *it) != compare_greater) {
        continue;
      }
      qmin = it;
    }
    return *qmin;
  }

G_array std_array_reverse(const G_array& data)
  {
    // This is an easy matter, isn't it?
    return G_array(data.rbegin(), data.rend());
  }

G_array std_array_generate(const Global_Context& global, const G_function& generator, const G_integer& length)
  {
    G_array res;
    res.reserve(static_cast<size_t>(length));
    for(int64_t i = 0; i < length; ++i) {
      // Set up arguments for the user-defined generator.
      cow_vector<Reference> args;
      do_push_argument(args, G_integer(i));
      do_push_argument(args, res.empty() ? null_value : res.back());
      // Call the generator function and push the return value.
      Reference self;
      generator->invoke(self, global, ::rocket::move(args));
      self.finish_call(global);
      res.emplace_back(self.read());
    }
    return res;
  }

G_array std_array_shuffle(const G_array& data, const opt<G_integer>& seed)
  {
    if(data.size() <= 1) {
      // Use reference counting as our advantage.
      return data;
    }
    // Create a linear congruential generator.
    auto lcgst = seed ? static_cast<uint64_t>(*seed) : generate_random_seed();
    // Shuffle elements.
    G_array res = data;
    for(size_t i = 0; i < res.size(); ++i) {
      // These arguments are the same as glibc's `drand48()` function.
      //   https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
      lcgst *= 0x5DEECE66D;     // a
      lcgst += 0xB;             // c
      lcgst &= 0xFFFFFFFFFFFF;  // m
      // N.B. Conversion from an unsigned type to a floating-point type would result in performance penalty.
      // ratio <= [0.0, 1.0)
      auto ratio = static_cast<double>(static_cast<int64_t>(lcgst)) / 0x1p48;
      // k <= [0, res.size())
      auto k = static_cast<size_t>(static_cast<int64_t>(ratio * static_cast<double>(data.ssize())));
      if(k == i) {
        continue;
      }
      xswap(res.mut(k), res.mut(i));
    }
    return res;
  }

opt<G_array> std_array_copy_keys(const opt<G_object>& source)
  {
    if(!source) {
      return ::rocket::clear;
    }
    G_array res;
    res.reserve(source->size());
    ::rocket::for_each(*source, [&](const auto& p) { res.emplace_back(G_string(p.first));  });
    return res;
  }

opt<G_array> std_array_copy_values(const opt<G_object>& source)
  {
    if(!source) {
      return ::rocket::clear;
    }
    G_array res;
    res.reserve(source->size());
    ::rocket::for_each(*source, [&](const auto& p) { res.emplace_back(p.second);  });
    return res;
  }

void create_bindings_array(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.array.slice()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("slice"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.slice(data, from, [length])`\n"
          "\n"
          "  * Copies a subrange of `data` to create a new `array`. Elements\n"
          "    are copied from `from` if it is non-negative, or from\n"
          "    `lengthof(data) + from` otherwise. If `length` is set to an\n"
          "    `integer`, no more than this number of elements will be copied.\n"
          "    If it is absent, all elements from `from` to the end of `data`\n"
          "    will be copied. If `from` is outside `data`, an empty `array`\n"
          "    is returned.\n"
          "\n"
          "  * Returns the specified subarray of `data`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.slice"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          G_integer from;
          opt<G_integer> length;
          if(reader.start().g(data).g(from).g(length).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_slice(data, from, length) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.replace_slice()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("replace_slice"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.replace_slice(data, from, replacement)`\n"
          "\n"
          "  * Replaces all elements from `from` to the end of `data` with\n"
          "    `replacement` and returns the new `array`. If `from` is\n"
          "    negative, it specifies an offset from the end of `data`. This\n"
          "    function returns a new `array` without modifying `data`.\n"
          "\n"
          "  * Returns a `array` with the subrange replaced.\n"
          "\n"
          "`std.array.replace_slice(data, from, [length], replacement)`\n"
          "\n"
          "  * Replaces a subrange of `data` with `replacement` to create a\n"
          "    new `array`. `from` specifies the start of the subrange to\n"
          "    replace. If `from` is negative, it specifies an offset from the\n"
          "    end of `data`. `length` specifies the maximum number of\n"
          "    elements to replace. If it is set to `null`, this function is\n"
          "    equivalent to `replace_slice(data, from, replacement)`. This\n"
          "    function returns a new `array` without modifying `data`.\n"
          "\n"
          "  * Returns a `array` with the subrange replaced.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.replace"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          G_array data;
          G_integer from;
          G_array replacement;
          if(reader.start().g(data).g(from).save(state).g(replacement).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_replace_slice(data, from, replacement) };
            return ::rocket::move(xref);
          }
          opt<G_integer> length;
          if(reader.load(state).g(length).g(replacement).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_replace_slice(data, from, length, replacement) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.find()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.find(data, target)`\n"
          "\n"
          "  * Searches `data` for the first occurrence of `target`.\n"
          "\n"
          "  * Returns the subscript of the first match of `target` in `data`\n"
          "    if one is found, which is always non-negative, or `null`\n"
          "    otherwise.\n"
          "\n"
          "`std.array.find(data, from, target)`\n"
          "\n"
          "  * Searches `data` for the first occurrence of `target`. The\n"
          "    search operation is performed on the same subrange that would\n"
          "    be returned by `slice(data, from)`.\n"
          "\n"
          "  * Returns the subscript of the first match of `target` in `data`\n"
          "    if one is found, which is always non-negative, or `null`\n"
          "    otherwise.\n"
          "\n"
          "`std.array.find(data, from, [length], target)`\n"
          "\n"
          "  * Searches `data` for the first occurrence of `target`. The\n"
          "    search operation is performed on the same subrange that would\n"
          "    be returned by `slice(data, from, length)`.\n"
          "\n"
          "  * Returns the subscript of the first match of `target` in `data`\n"
          "    if one is found, which is always non-negative, or `null`\n"
          "    otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.find"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          G_array data;
          Value target;
          if(reader.start().g(data).save(state).g(target).finish()) {
            // Call the binding function.
            auto qindex = std_array_find(data, target);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          G_integer from;
          if(reader.load(state).g(from).save(state).g(target).finish()) {
            // Call the binding function.
            auto qindex = std_array_find(data, from, target);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          opt<G_integer> length;
          if(reader.load(state).g(length).g(target).finish()) {
            // Call the binding function.
            auto qindex = std_array_find(data, from, length, target);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.find_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_if"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.find_if(data, predictor)`\n"
          "\n"
          "  * Finds the first element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically true.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
          "\n"
          "`std.array.find_if(data, from, predictor)`\n"
          "\n"
          "  * Finds the first element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically true. The search operation is\n"
          "    performed on the same subrange that would be returned by\n"
          "    `slice(data, from)`.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
          "\n"
          "`std.array.find_if(data, from, [length], predictor)`\n"
          "\n"
          "  * Finds the first element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically true. The search operation is\n"
          "    performed on the same subrange that would be returned by\n"
          "    `slice(data, from, length)`.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.find_if"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          G_array data;
          G_function predictor;
          if(reader.start().g(data).save(state).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_find_if(global, data, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          G_integer from;
          if(reader.load(state).g(from).save(state).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_find_if(global, data, from, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          opt<G_integer> length;
          if(reader.load(state).g(length).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_find_if(global, data, from, length, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.find_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_if_not"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.find_if_not(data, predictor)`\n"
          "\n"
          "  * Finds the first element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically false.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
          "\n"
          "`std.array.find_if_not(data, from, predictor)`\n"
          "\n"
          "  * Finds the first element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically false. The search operation is\n"
          "    performed on the same subrange that would be returned by\n"
          "    `slice(data, from)`.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
          "\n"
          "`std.array.find_if_not(data, from, [length], predictor)`\n"
          "\n"
          "  * Finds the first element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically false. The search operation is\n"
          "    performed on the same subrange that would be returned by\n"
          "    `slice(data, from, length)`.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.find_if_not"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          G_array data;
          G_function predictor;
          if(reader.start().g(data).save(state).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_find_if_not(global, data, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          G_integer from;
          if(reader.load(state).g(from).save(state).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_find_if_not(global, data, from, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          opt<G_integer> length;
          if(reader.load(state).g(length).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_find_if_not(global, data, from, length, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.rfind()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.rfind(data, target)`\n"
          "\n"
          "  * Searches `data` for the last occurrence of `target`.\n"
          "\n"
          "  * Returns the subscript of the last match of `target` in `data`\n"
          "    if one is found, which is always non-negative, or `null`\n"
          "    otherwise.\n"
          "\n"
          "`std.array.rfind(data, from, target)`\n"
          "\n"
          "  * Searches `data` for the last occurrence of `target`. The search\n"
          "    operation is performed on the same subrange that would be\n"
          "    returned by `slice(data, from)`.\n"
          "\n"
          "  * Returns the subscript of the last match of `target` in `data`\n"
          "    if one is found, which is always non-negative, or `null`\n"
          "    otherwise.\n"
          "\n"
          "`std.array.rfind(data, from, [length], target)`\n"
          "\n"
          "  * Searches `data` for the last occurrence of `target`. The search\n"
          "    operation is performed on the same subrange that would be\n"
          "    returned by `slice(data, from, length)`.\n"
          "\n"
          "  * Returns the subscript of the last match of `target` in `data`\n"
          "    if one is found, which is always non-negative, or `null`\n"
          "    otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.rfind"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          G_array data;
          Value target;
          if(reader.start().g(data).save(state).g(target).finish()) {
            // Call the binding function.
            auto qindex = std_array_rfind(data, target);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          G_integer from;
          if(reader.load(state).g(from).save(state).g(target).finish()) {
            // Call the binding function.
            auto qindex = std_array_rfind(data, from, target);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          opt<G_integer> length;
          if(reader.load(state).g(length).g(target).finish()) {
            // Call the binding function.
            auto qindex = std_array_rfind(data, from, length, target);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.rfind_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind_if"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.rfind_if(data, predictor)`\n"
          "\n"
          "  * Finds the last element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically true.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
          "\n"
          "`std.array.rfind_if(data, from, predictor)`\n"
          "\n"
          "  * Finds the last element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically true. The search operation is\n"
          "    performed on the same subrange that would be returned by\n"
          "    `slice(data, from)`.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
          "\n"
          "`std.array.rfind_if(data, from, [length], predictor)`\n"
          "\n"
          "  * Finds the last element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically true. The search operation is\n"
          "    performed on the same subrange that would be returned by\n"
          "    `slice(data, from, length)`.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.rfind_if"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          G_array data;
          G_function predictor;
          if(reader.start().g(data).save(state).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_rfind_if(global, data, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          G_integer from;
          if(reader.load(state).g(from).save(state).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_rfind_if(global, data, from, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          opt<G_integer> length;
          if(reader.load(state).g(length).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_rfind_if(global, data, from, length, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.rfind_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind_if_not"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.rfind_if_not(data, predictor)`\n"
          "\n"
          "  * Finds the last element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically false.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
          "\n"
          "`std.array.rfind_if_not(data, from, predictor)`\n"
          "\n"
          "  * Finds the last element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically false. The search operation is\n"
          "    performed on the same subrange that would be returned by\n"
          "    `slice(data, from)`.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
          "\n"
          "`std.array.rfind_if_not(data, from, [length], predictor)`\n"
          "\n"
          "  * Finds the last element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically false. The search operation is\n"
          "    performed on the same subrange that would be returned by\n"
          "    `slice(data, from, length)`.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.rfind_if_not"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          G_array data;
          G_function predictor;
          if(reader.start().g(data).save(state).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_rfind_if_not(global, data, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          G_integer from;
          if(reader.load(state).g(from).save(state).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_rfind_if_not(global, data, from, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          opt<G_integer> length;
          if(reader.load(state).g(length).g(predictor).finish()) {
            // Call the binding function.
            auto qindex = std_array_rfind_if_not(global, data, from, length, predictor);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.count()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("count"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.count(data, target)`\n"
          "\n"
          "  * Searches `data` for `target` and figures the total number of\n"
          "    occurences.\n"
          "\n"
          "  * Returns the number of occurrences as an `integer`, which is\n"
          "    always non-negative.\n"
          "\n"
          "`std.array.count(data, from, target)`\n"
          "\n"
          "  * Searches `data` for `target` and figures the total number of\n"
          "    occurences. The search operation is performed on the same\n"
          "    subrange that would be returned by `slice(data, from)`.\n"
          "\n"
          "  * Returns the number of occurrences as an `integer`, which is\n"
          "    always non-negative.\n"
          "\n"
          "`std.array.count(data, from, [length], target)`\n"
          "\n"
          "  * Searches `data` for `target` and figures the total number of\n"
          "    occurences. The search operation is performed on the same\n"
          "    subrange that would be returned by `slice(data, from, length)`.\n"
          "\n"
          "  * Returns the number of occurrences as an `integer`, which is\n"
          "    always non-negative.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.count"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          G_array data;
          Value target;
          if(reader.start().g(data).save(state).g(target).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_count(data, target) };
            return ::rocket::move(xref);
          }
          G_integer from;
          if(reader.load(state).g(from).save(state).g(target).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_count(data, from, target) };
            return ::rocket::move(xref);
          }
          opt<G_integer> length;
          if(reader.load(state).g(length).g(target).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_count(data, from, length, target) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.count_if()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("count_if"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.count_if(data, target, predictor)`\n"
          "\n"
          "  * Searches every element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically true, and figures the total\n"
          "    number of such occurences.\n"
          "\n"
          "  * Returns the number of occurrences as an `integer`, which is\n"
          "    always non-negative.\n"
          "\n"
          "`std.array.count_if(data, from, target, predictor)`\n"
          "\n"
          "  * Searches every element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically true, and figures the total\n"
          "    number of elements. The search operation is performed on the\n"
          "    same subrange that would be returned by `slice(data, from)`.\n"
          "\n"
          "  * Returns the number of occurrences as an `integer`, which is\n"
          "    always non-negative.\n"
          "\n"
          "`std.array.count_if(data, from, [length], target, predictor)`\n"
          "\n"
          "  * Searches every element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically true, and figures the total\n"
          "    number of elements. The search operation is performed on the\n"
          "    same subrange that would be returned by `slice(data, from,\n"
          "    length)`.\n"
          "  * Returns the number of occurrences as an `integer`, which is\n"
          "    always non-negative.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.count_if"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          G_array data;
          G_function predictor;
          if(reader.start().g(data).save(state).g(predictor).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_count_if(global, data, predictor) };
            return ::rocket::move(xref);
          }
          G_integer from;
          if(reader.load(state).g(from).save(state).g(predictor).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_count_if(global, data, from, predictor) };
            return ::rocket::move(xref);
          }
          opt<G_integer> length;
          if(reader.load(state).g(length).g(predictor).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_count_if(global, data, from, length, predictor) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.count_if_not()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("count_if_not"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.count_if_not(data, target, predictor)`\n"
          "\n"
          "  * Searches every element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically false, and figures the total\n"
          "    number of such occurences.\n"
          "\n"
          "  * Returns the number of occurrences as an `integer`, which is\n"
          "    always non-negative.\n"
          "\n"
          "`std.array.count_if_not(data, from, target, predictor)`\n"
          "\n"
          "  * Searches every element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically false, and figures the total\n"
          "    number of elements. The search operation is performed on the\n"
          "    same subrange that would be returned by `slice(data, from)`.\n"
          "\n"
          "  * Returns the number of occurrences as an `integer`, which is\n"
          "    always non-negative.\n"
          "\n"
          "`std.array.count_if_not(data, from, [length], target, predictor)`\n"
          "\n"
          "  * Searches every element, namely `x`, in `data`, for which\n"
          "    `predictor(x)` yields logically false, and figures the total\n"
          "    number of elements. The search operation is performed on the\n"
          "    same subrange that would be returned by `slice(data, from,\n"
          "    length)`.\n"
          "\n"
          "  * Returns the number of occurrences as an `integer`, which is\n"
          "    always non-negative.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.count_if_not"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          G_array data;
          G_function predictor;
          if(reader.start().g(data).save(state).g(predictor).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_count_if_not(global, data, predictor) };
            return ::rocket::move(xref);
          }
          G_integer from;
          if(reader.load(state).g(from).save(state).g(predictor).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_count_if_not(global, data, from, predictor) };
            return ::rocket::move(xref);
          }
          opt<G_integer> length;
          if(reader.load(state).g(length).g(predictor).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_count_if_not(global, data, from, length, predictor) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.is_sorted()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("is_sorted"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.is_sorted(data, [comparator])`\n"
          "\n"
          "  * Checks whether `data` is sorted. That is, there is no pair of\n"
          "    adjacent elements in `data` such that the first one is greater\n"
          "    than or unordered with the second one. Elements are compared\n"
          "    using `comparator`, which shall be a binary `function` that\n"
          "    returns a negative `integer` or `real` if the first argument is\n"
          "    less than the second one, a positive `integer` or `real` if the\n"
          "    first argument is greater than the second one, or `0` if the\n"
          "    arguments are equal; other values indicate that the arguments\n"
          "    are unordered. If no `comparator` is provided, the built-in\n"
          "    3-way comparison operator is used. An `array` that contains no\n"
          "    elements is considered to have been sorted.\n"
          "\n"
          "  * Returns `true` if `data` is sorted or empty, or `false`\n"
          "    otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.is_sorted"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          opt<G_function> comparator;
          if(reader.start().g(data).g(comparator).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_is_sorted(global, data, comparator) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.binary_search()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("binary_search"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.binary_search(data, target, [comparator])`\n"
          "\n"
          "  * Finds the first element in `data` that is equal to `target`.\n"
          "    The principle of user-defined `comparator`s is the same as the\n"
          "    `is_sorted()` function. As a consequence, the function call\n"
          "    `is_sorted(data, comparator)` shall yield `true` prior to this\n"
          "    call, otherwise the effect is undefined.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`, if\n"
          "    one is found, or `null` otherwise.\n"
          "\n"
          "  * Throws an exception if `data` has not been sorted properly. Be\n"
          "    advised that in this case there is no guarantee whether an\n"
          "    exception will be thrown or not.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.binary_search"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          Value target;
          opt<G_function> comparator;
          if(reader.start().g(data).g(target).g(comparator).finish()) {
            // Call the binding function.
            auto qindex = std_array_binary_search(global, data, target, comparator);
            if(!qindex) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qindex) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.lower_bound()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("lower_bound"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.lower_bound(data, target, [comparator])`\n"
          "\n"
          "  * Finds the first element in `data` that is greater than or equal\n"
          "    to `target` and precedes all elements that are less than\n"
          "    `target` if any. The principle of user-defined `comparator`s is\n"
          "    the same as the `is_sorted()` function. As a consequence, the\n"
          "    function call `is_sorted(data, comparator)` shall yield `true`\n"
          "    prior to this call, otherwise the effect is undefined.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`. This\n"
          "    function returns `lengthof(data)` if all elements are less than\n"
          "    `target`.\n"
          "\n"
          "  * Throws an exception if `data` has not been sorted properly. Be\n"
          "    advised that in this case there is no guarantee whether an\n"
          "    exception will be thrown or not.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.lower_bound"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          Value target;
          opt<G_function> comparator;
          if(reader.start().g(data).g(target).g(comparator).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_lower_bound(global, data, target, comparator) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.upper_bound()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("upper_bound"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.upper_bound(data, target, [comparator])`\n"
          "\n"
          "  * Finds the first element in `data` that is greater than `target`\n"
          "    and precedes all elements that are less than or equal to\n"
          "    `target` if any. The principle of user-defined `comparator`s is\n"
          "    the same as the `is_sorted()` function. As a consequence, the\n"
          "    function call `is_sorted(data, comparator)` shall yield `true`\n"
          "    prior to this call, otherwise the effect is undefined.\n"
          "\n"
          "  * Returns the subscript of such an element as an `integer`. This\n"
          "    function returns `lengthof(data)` if all elements are less than\n"
          "    or equal to `target`.\n"
          "\n"
          "  * Throws an exception if `data` has not been sorted properly. Be\n"
          "    advised that in this case there is no guarantee whether an\n"
          "    exception will be thrown or not.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.upper_bound"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          Value target;
          opt<G_function> comparator;
          if(reader.start().g(data).g(target).g(comparator).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_upper_bound(global, data, target, comparator) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.equal_range()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("equal_range"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.equal_range(data, target, [comparator])`\n"
          "\n"
          "  * Gets the range of elements equivalent to `target` in `data` as\n"
          "    a single function call. This function is equivalent to calling\n"
          "    `lower_bound(data, target, comparator)` and\n"
          "    `upper_bound(data, target, comparator)` respectively then\n"
          "    storing both results in an `array`.\n"
          "\n"
          "  * Returns an `array` of two `integer`s, the first of which\n"
          "    specifies the lower bound and the other specifies the upper\n"
          "    bound.\n"
          "\n"
          "  * Throws an exception if `data` has not been sorted properly. Be\n"
          "    advised that in this case there is no guarantee whether an\n"
          "    exception will be thrown or not.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.lower_bound"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          Value target;
          opt<G_function> comparator;
          if(reader.start().g(data).g(target).g(comparator).finish()) {
            // Call the binding function.
            auto pair = std_array_equal_range(global, data, target, comparator);
            // This function returns a `pair`, but we would like to return an array so convert it.
            G_array xval;
            xval.emplace_back(pair.first);
            xval.emplace_back(pair.second);
            // Return the array.
            Reference_Root::S_temporary xref = { ::rocket::move(xval) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.sort()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sort"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.sort(data, [comparator])`\n"
          "\n"
          "  * Sorts elements in `data` in ascending order. The principle of\n"
          "    user-defined `comparator`s is the same as the `is_sorted()`\n"
          "    function. The algorithm shall finish in `O(n log n)` time where\n"
          "    `n` is the number of elements in `data`, and shall be stable.\n"
          "    This function returns a new `array` without modifying `data`.\n"
          "\n"
          "  * Returns the sorted `array`.\n"
          "\n"
          "  * Throws an exception if any elements are unordered. Be advised\n"
          "    that in this case there is no guarantee whether an exception\n"
          "    will be thrown or not.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.sort"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          opt<G_function> comparator;
          if(reader.start().g(data).g(comparator).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_sort(global, data, comparator) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.sortu()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("sortu"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.sortu(data, [comparator])`\n"
          "\n"
          "  * Sorts elements in `data` in ascending order, then removes all\n"
          "    elements that have preceding equivalents. The principle of\n"
          "    user-defined `comparator`s is the same as the `is_sorted()`\n"
          "    function. The algorithm shall finish in `O(n log n)` time where\n"
          "    `n` is the number of elements in `data`. This function returns\n"
          "    a new `array` without modifying `data`.\n"
          "\n"
          "  * Returns the sorted `array` with no duplicate elements.\n"
          "\n"
          "  * Throws an exception if any elements are unordered. Be advised\n"
          "    that in this case there is no guarantee whether an exception\n"
          "    will be thrown or not.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.sortu"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          opt<G_function> comparator;
          if(reader.start().g(data).g(comparator).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_sortu(global, data, comparator) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.max_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("max_of"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.max_of(data, [comparator])`\n"
          "\n"
          "  * Finds the maximum element in `data`. The principle of\n"
          "    user-defined `comparator`s is the same as the `is_sorted()`\n"
          "    function. Elements that are unordered with the first element\n"
          "    are ignored silently.\n"
          "\n"
          "  * Returns a copy of the maximum element, or `null` if `data` is\n"
          "    empty.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.max_of"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          opt<G_function> comparator;
          if(reader.start().g(data).g(comparator).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_max_of(global, data, comparator) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.min_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("min_of"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.min_of(data, [comparator])`\n"
          "\n"
          "  * Finds the minimum element in `data`. The principle of\n"
          "    user-defined `comparator`s is the same as the `is_sorted()`\n"
          "    function. Elements that are unordered with the first element\n"
          "    are ignored silently.\n"
          "\n"
          "  * Returns a copy of the minimum element, or `null` if `data` is\n"
          "    empty.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.min_of"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          opt<G_function> comparator;
          if(reader.start().g(data).g(comparator).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_min_of(global, data, comparator) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.reverse()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("reverse"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.reverse(data)`\n"
          "\n"
          "  * Reverses an `array`. This function returns a new `array`\n"
          "    without modifying `text`.\n"
          "\n"
          "  * Returns the reversed `array`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.reverse"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          opt<G_function> comparator;
          if(reader.start().g(data).g(comparator).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_reverse(data) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.generate()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("generate"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.generate(generator, length)`\n"
          "\n"
          "  * Calls `generator` repeatedly up to `length` times and returns\n"
          "    an `array` consisting of all values returned. `generator` shall\n"
          "    be a binary function. The first argument will be the number of\n"
          "    elements having been generated; the second argument is the\n"
          "    previous element generated, or `null` in the case of the first\n"
          "    element.\n"
          "\n"
          "  * Returns an `array` containing all values generated.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args, Reference&& /*self*/, const Global_Context& global) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.generate"), ::rocket::ref(args));
          // Parse arguments.
          G_function generator;
          G_integer length;
          if(reader.start().g(generator).g(length).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_generate(global, generator, length) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.shuffle()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("shuffle"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.shuffle(data, [seed])`\n"
          "\n"
          "  * Shuffles elements in `data` randomly. If `seed` is set to an\n"
          "    `integer`, the internal pseudo random number generator will be\n"
          "    initialized with it and will produce the same series of numbers\n"
          "    for a specific `seed` value. If it is absent, an unspecified\n"
          "    seed is generated when this function is called. This function\n"
          "    returns a new `array` without modifying `data`.\n"
          "\n"
          "  * Returns the shuffled `array`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.shuffle"), ::rocket::ref(args));
          // Parse arguments.
          G_array data;
          opt<G_integer> seed;
          if(reader.start().g(data).g(seed).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_array_shuffle(data, seed) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.copy_keys()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("copy_keys"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.copy_keys([source])`\n"
          "\n"
          "  * Copies all keys from `source`, which shall be an `object`, to\n"
          "    create an `array`.\n"
          "\n"
          "  * Returns an `array` of all keys in `source`. If `source` is\n"
          "    `null`, `null` is returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.copy_keys"), ::rocket::ref(args));
          // Parse arguments.
          opt<G_object> source;
          if(reader.start().g(source).finish()) {
            // Call the binding function.
            auto qres = std_array_copy_keys(source);
            if(!qres) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qres) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.array.copy_values()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("copy_values"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.array.copy_values([source])`\n"
          "\n"
          "  * Copies all values from `source`, which shall be an `object`, to\n"
          "    create an `array`.\n"
          "\n"
          "  * Returns an `array` of all values in `source`. If `source` is\n"
          "    `null`, `null` is returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.array.copy_values"), ::rocket::ref(args));
          // Parse arguments.
          opt<G_object> source;
          if(reader.start().g(source).finish()) {
            // Call the binding function.
            auto qres = std_array_copy_values(source);
            if(!qres) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qres) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // End of `std.array`
    //===================================================================
  }

}  // namespace Asteria
