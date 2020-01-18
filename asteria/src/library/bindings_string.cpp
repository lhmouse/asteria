// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_string.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

namespace Asteria {
namespace {

using Slice = pair<Sval::const_iterator, Sval::const_iterator>;

Slice do_slice(const Sval& text, Sval::const_iterator tbegin, const Iopt& length)
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

Slice do_slice(const Sval& text, const Ival& from, const Iopt& length)
  {
    auto slen = static_cast<int64_t>(text.size());
    if(from >= 0) {
      // Behave like `::std::string::substr()` except that no exception is thrown when `from` is
      // greater than `text.size()`.
      if(from >= slen) {
        return ::std::make_pair(text.end(), text.end());
      }
      return do_slice(text, text.begin() + static_cast<ptrdiff_t>(from), length);
    }
    // Wrap `from` from the end. Notice that `from + slen` will not overflow when `from` is negative
    // and `slen` is not.
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

template<typename IteratorT> opt<IteratorT> do_find_opt(IteratorT tbegin, IteratorT tend,
                                                        IteratorT pbegin, IteratorT pend)
  {
    // https://en.wikipedia.org/wiki/Boyer-Moore-Horspool_algorithm
    auto plen = ::std::distance(pbegin, pend);
    if(plen <= 0) {
      // Return a match at the the beginning if the pattern is empty.
      return tbegin;
    }
    // Build a table according to the Bad Character Rule.
    array<ptrdiff_t, 0x100> bcr_table;
    for(size_t i = 0;  i < 0x100;  ++i) {
      bcr_table[i] = plen;
    }
    for(ptrdiff_t i = plen - 1;  i != 0;  --i) {
      bcr_table[(pend[~i] & 0xFF)] = i;
    }
    // Search for the pattern.
    auto tpos = tbegin;
    for(;;) {
      if(tend - tpos < plen) {
        return emptyc;
      }
      if(::std::equal(pbegin, pend, tpos)) {
        break;
      }
      tpos += bcr_table[(tpos[(plen - 1)] & 0xFF)];
    }
    return ::rocket::move(tpos);
  }

template<typename IteratorT> opt<IteratorT> do_find_of_opt(IteratorT begin, IteratorT end,
                                                           const Sval& set, bool match)
  {
    // Make a lookup table.
    array<bool, 256> table = { };
    ::rocket::for_each(set, [&](char c) { table[uint8_t(c)] = true;  });
    // Search the range.
    for(auto it = ::rocket::move(begin);  it != end;  ++it) {
      if(table[uint8_t(*it)] == match)
        return ::rocket::move(it);
    }
    return emptyc;
  }

inline Sval do_get_reject(const Sopt& reject)
  {
    if(!reject) {
      return ::rocket::sref(" \t");
    }
    return *reject;
  }

inline Sval do_get_padding(const Sopt& padding)
  {
    if(!padding) {
      return ::rocket::sref(" ");
    }
    if(padding->empty()) {
      ASTERIA_THROW("empty padding string not valid");
    }
    return *padding;
  }

constexpr char s_char_table[][2] =
  {
    "\x00", "\x01", "\x02", "\x03", "\x04", "\x05", "\x06", "\x07",
    "\x08", "\x09", "\x0A", "\x0B", "\x0C", "\x0D", "\x0E", "\x0F",
    "\x10", "\x11", "\x12", "\x13", "\x14", "\x15", "\x16", "\x17",
    "\x18", "\x19", "\x1A", "\x1B", "\x1C", "\x1D", "\x1E", "\x1F",
    "\x20", "\x21", "\x22", "\x23", "\x24", "\x25", "\x26", "\x27",
    "\x28", "\x29", "\x2A", "\x2B", "\x2C", "\x2D", "\x2E", "\x2F",
    "\x30", "\x31", "\x32", "\x33", "\x34", "\x35", "\x36", "\x37",
    "\x38", "\x39", "\x3A", "\x3B", "\x3C", "\x3D", "\x3E", "\x3F",
    "\x40", "\x41", "\x42", "\x43", "\x44", "\x45", "\x46", "\x47",
    "\x48", "\x49", "\x4A", "\x4B", "\x4C", "\x4D", "\x4E", "\x4F",
    "\x50", "\x51", "\x52", "\x53", "\x54", "\x55", "\x56", "\x57",
    "\x58", "\x59", "\x5A", "\x5B", "\x5C", "\x5D", "\x5E", "\x5F",
    "\x60", "\x61", "\x62", "\x63", "\x64", "\x65", "\x66", "\x67",
    "\x68", "\x69", "\x6A", "\x6B", "\x6C", "\x6D", "\x6E", "\x6F",
    "\x70", "\x71", "\x72", "\x73", "\x74", "\x75", "\x76", "\x77",
    "\x78", "\x79", "\x7A", "\x7B", "\x7C", "\x7D", "\x7E", "\x7F",
    "\x80", "\x81", "\x82", "\x83", "\x84", "\x85", "\x86", "\x87",
    "\x88", "\x89", "\x8A", "\x8B", "\x8C", "\x8D", "\x8E", "\x8F",
    "\x90", "\x91", "\x92", "\x93", "\x94", "\x95", "\x96", "\x97",
    "\x98", "\x99", "\x9A", "\x9B", "\x9C", "\x9D", "\x9E", "\x9F",
    "\xA0", "\xA1", "\xA2", "\xA3", "\xA4", "\xA5", "\xA6", "\xA7",
    "\xA8", "\xA9", "\xAA", "\xAB", "\xAC", "\xAD", "\xAE", "\xAF",
    "\xB0", "\xB1", "\xB2", "\xB3", "\xB4", "\xB5", "\xB6", "\xB7",
    "\xB8", "\xB9", "\xBA", "\xBB", "\xBC", "\xBD", "\xBE", "\xBF",
    "\xC0", "\xC1", "\xC2", "\xC3", "\xC4", "\xC5", "\xC6", "\xC7",
    "\xC8", "\xC9", "\xCA", "\xCB", "\xCC", "\xCD", "\xCE", "\xCF",
    "\xD0", "\xD1", "\xD2", "\xD3", "\xD4", "\xD5", "\xD6", "\xD7",
    "\xD8", "\xD9", "\xDA", "\xDB", "\xDC", "\xDD", "\xDE", "\xDF",
    "\xE0", "\xE1", "\xE2", "\xE3", "\xE4", "\xE5", "\xE6", "\xE7",
    "\xE8", "\xE9", "\xEA", "\xEB", "\xEC", "\xED", "\xEE", "\xEF",
    "\xF0", "\xF1", "\xF2", "\xF3", "\xF4", "\xF5", "\xF6", "\xF7",
    "\xF8", "\xF9", "\xFA", "\xFB", "\xFC", "\xFD", "\xFE", "\xFF",
  };

constexpr char s_base16_table[] = "00112233445566778899AaBbCcDdEeFf";
constexpr char s_base32_table[] = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz223344556677==";
constexpr char s_base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/==";
constexpr char s_spaces[] = " \f\n\r\t\v";

const char* do_xstrchr(const char* str, char c) noexcept
  {
    // If `c == 0`, this function returns a null pointer.
    for(auto p = str;  *p != 0;  ++p)
      if(*p == c)
        return p;
    return nullptr;
  }

template<bool bigendT, typename WordT> Sval& do_pack_one_impl(Sval& text, const Ival& value)
  {
    // Define temporary storage.
    array<char, sizeof(WordT)> stor_le;
    uint64_t word = static_cast<uint64_t>(value);
    // Write it in little-endian order.
    for(size_t i = 0;  i < stor_le.size();  ++i) {
      stor_le[i] = static_cast<char>(word);
      word >>= 8;
    }
    // Append this word.
    if(bigendT)
      text.append(stor_le.rbegin(), stor_le.rend());
    else
      text.append(stor_le.begin(), stor_le.end());
    // Return  the output string.
    return text;
  }
template<typename WordT> Sval& do_pack_one_be(Sval& text, const Ival& value)
  {
    return do_pack_one_impl<1, WordT>(text, value);
  }
template<typename WordT> Sval& do_pack_one_le(Sval& text, const Ival& value)
  {
    return do_pack_one_impl<0, WordT>(text, value);
  }

template<bool bigendT, typename WordT> Aval do_unpack_impl(const Sval& text)
  {
    Aval values;
    // Define temporary storage.
    array<char, sizeof(WordT)> stor_be;
    uint64_t word = 0;
    // How many words will the result have?
    auto nwords = text.size() / stor_be.size();
    if(text.size() != nwords * stor_be.size()) {
      ASTERIA_THROW("invalid source string length (`$1` not divisible by `$2`)", text.size(), stor_be.size());
    }
    values.reserve(nwords);
    // Unpack integers.
    for(size_t i = 0;  i < nwords;  ++i) {
      // Read some bytes in big-endian order.
      if(bigendT)
        ::std::copy_n(text.data() + i * stor_be.size(), stor_be.size(), stor_be.mut_begin());
      else
        ::std::copy_n(text.data() + i * stor_be.size(), stor_be.size(), stor_be.mut_rbegin());
      // Assemble the word.
      for(const auto& byte : stor_be) {
        word <<= 8;
        word |= static_cast<uint8_t>(byte);
      }
      // Append the word.
      values.emplace_back(Ival(static_cast<WordT>(word)));
    }
    return values;
  }
template<typename WordT> Aval do_unpack_be(const Sval& text)
  {
    return do_unpack_impl<1, WordT>(text);
  }
template<typename WordT> Aval do_unpack_le(const Sval& text)
  {
    return do_unpack_impl<0, WordT>(text);
  }

}  // namespace

Sval std_string_slice(Sval text, Ival from, Iopt length)
  {
    auto range = do_slice(text, from, length);
    if((range.first == text.begin()) && (range.second == text.end())) {
      // Use reference counting as our advantage.
      return text;
    }
    return Sval(range.first, range.second);
  }

Sval std_string_replace_slice(Sval text, Ival from, Sval replacement)
  {
    Sval res = text;
    auto range = do_slice(res, from, emptyc);
    // Replace the subrange.
    res.replace(range.first, range.second, replacement);
    return res;
  }

Sval std_string_replace_slice(Sval text, Ival from, Iopt length, Sval replacement)
  {
    Sval res = text;
    auto range = do_slice(res, from, length);
    // Replace the subrange.
    res.replace(range.first, range.second, replacement);
    return res;
  }

Ival std_string_compare(Sval text1, Sval text2, Iopt length)
  {
    if(!length || (*length >= PTRDIFF_MAX)) {
      // Compare the entire strings.
      return text1.compare(text2);
    }
    if(*length <= 0) {
      // There is nothing to compare.
      return 0;
    }
    // Compare two substrings.
    return text1.compare(0, static_cast<size_t>(*length), text2, 0, static_cast<size_t>(*length));
  }

Bval std_string_starts_with(Sval text, Sval prefix)
  {
    return text.starts_with(prefix);
  }

Bval std_string_ends_with(Sval text, Sval suffix)
  {
    return text.ends_with(suffix);
  }

Iopt std_string_find(Sval text, Sval pattern)
  {
    auto range = ::std::make_pair(text.begin(), text.end());
    auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
    if(!qit) {
      return emptyc;
    }
    return *qit - text.begin();
  }

Iopt std_string_find(Sval text, Ival from, Sval pattern)
  {
    auto range = do_slice(text, from, emptyc);
    auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
    if(!qit) {
      return emptyc;
    }
    return *qit - text.begin();
  }

Iopt std_string_find(Sval text, Ival from, Iopt length, Sval pattern)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
    if(!qit) {
      return emptyc;
    }
    return *qit - text.begin();
  }

Iopt std_string_rfind(Sval text, Sval pattern)
  {
    auto range = ::std::make_pair(text.begin(), text.end());
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first),
                           pattern.rbegin(), pattern.rend());
    if(!qit) {
      return emptyc;
    }
    return text.rend() - *qit - pattern.ssize();
  }

Iopt std_string_rfind(Sval text, Ival from, Sval pattern)
  {
    auto range = do_slice(text, from, emptyc);
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first),
                           pattern.rbegin(), pattern.rend());
    if(!qit) {
      return emptyc;
    }
    return text.rend() - *qit - pattern.ssize();
  }

Iopt std_string_rfind(Sval text, Ival from, Iopt length, Sval pattern)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first),
                           pattern.rbegin(), pattern.rend());
    if(!qit) {
      return emptyc;
    }
    return text.rend() - *qit - pattern.ssize();
  }

Sval std_string_find_and_replace(Sval text, Sval pattern, Sval replacement)
  {
    Sval res = text;
    auto range = ::std::make_pair(res.begin(), res.end());
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
      if(!qit) {
        // Make use of reference counting if no match has been found.
        break;
      }
      // Save offsets before replacing due to possibility of invalidation of iterators.
      auto roffs = ::std::make_pair(*qit - res.begin(), range.second - res.begin());
      res.replace(*qit, *qit + pattern.ssize(), replacement);
      range = ::std::make_pair(res.begin() + roffs.first + replacement.ssize(), res.begin() + roffs.second);
    }
    return res;
  }

Sval std_string_find_and_replace(Sval text, Ival from, Sval pattern, Sval replacement)
  {
    Sval res = text;
    auto range = do_slice(res, from, emptyc);
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
      if(!qit) {
        // Make use of reference counting if no match has been found.
        break;
      }
      // Save offsets before replacing due to possibility of invalidation of iterators.
      auto roffs = ::std::make_pair(*qit - res.begin(), range.second - res.begin());
      res.replace(*qit, *qit + pattern.ssize(), replacement);
      range = ::std::make_pair(res.begin() + roffs.first + replacement.ssize(), res.begin() + roffs.second);
    }
    return res;
  }

Sval std_string_find_and_replace(Sval text, Ival from, Iopt length, Sval pattern,
                                 Sval replacement)
  {
    Sval res = text;
    auto range = do_slice(res, from, length);
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
      if(!qit) {
        // Make use of reference counting if no match has been found.
        break;
      }
      // Save offsets before replacing due to possibility of invalidation of iterators.
      auto roffs = ::std::make_pair(*qit - res.begin(), range.second - res.begin());
      res.replace(*qit, *qit + pattern.ssize(), replacement);
      range = ::std::make_pair(res.begin() + roffs.first + replacement.ssize(), res.begin() + roffs.second);
    }
    return res;
  }

Iopt std_string_find_any_of(Sval text, Sval accept)
  {
    auto qit = do_find_of_opt(text.begin(), text.end(), accept, true);
    if(!qit) {
      return emptyc;
    }
    return *qit - text.begin();
  }

Iopt std_string_find_any_of(Sval text, Ival from, Sval accept)
  {
    auto range = do_slice(text, from, emptyc);
    auto qit = do_find_of_opt(range.first, range.second, accept, true);
    if(!qit) {
      return emptyc;
    }
    return *qit - text.begin();
  }

Iopt std_string_find_any_of(Sval text, Ival from, Iopt length, Sval accept)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(range.first, range.second, accept, true);
    if(!qit) {
      return emptyc;
    }
    return *qit - text.begin();
  }

Iopt std_string_find_not_of(Sval text, Sval reject)
  {
    auto qit = do_find_of_opt(text.begin(), text.end(), reject, false);
    if(!qit) {
      return emptyc;
    }
    return *qit - text.begin();
  }

Iopt std_string_find_not_of(Sval text, Ival from, Sval reject)
  {
    auto range = do_slice(text, from, emptyc);
    auto qit = do_find_of_opt(range.first, range.second, reject, false);
    if(!qit) {
      return emptyc;
    }
    return *qit - text.begin();
  }

Iopt std_string_find_not_of(Sval text, Ival from, Iopt length, Sval reject)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(range.first, range.second, reject, false);
    if(!qit) {
      return emptyc;
    }
    return *qit - text.begin();
  }

Iopt std_string_rfind_any_of(Sval text, Sval accept)
  {
    auto qit = do_find_of_opt(text.rbegin(), text.rend(), accept, true);
    if(!qit) {
      return emptyc;
    }
    return text.rend() - *qit - 1;
  }

Iopt std_string_rfind_any_of(Sval text, Ival from, Sval accept)
  {
    auto range = do_slice(text, from, emptyc);
    auto qit = do_find_of_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first),
                              accept, true);
    if(!qit) {
      return emptyc;
    }
    return text.rend() - *qit - 1;
  }

Iopt std_string_rfind_any_of(Sval text, Ival from, Iopt length, Sval accept)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first),
                              accept, true);
    if(!qit) {
      return emptyc;
    }
    return text.rend() - *qit - 1;
  }

Iopt std_string_rfind_not_of(Sval text, Sval reject)
  {
    auto qit = do_find_of_opt(text.rbegin(), text.rend(), reject, false);
    if(!qit) {
      return emptyc;
    }
    return text.rend() - *qit - 1;
  }

Iopt std_string_rfind_not_of(Sval text, Ival from, Sval reject)
  {
    auto range = do_slice(text, from, emptyc);
    auto qit = do_find_of_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first),
                              reject, false);
    if(!qit) {
      return emptyc;
    }
    return text.rend() - *qit - 1;
  }

Iopt std_string_rfind_not_of(Sval text, Ival from, Iopt length, Sval reject)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first),
                              reject, false);
    if(!qit) {
      return emptyc;
    }
    return text.rend() - *qit - 1;
  }

Sval std_string_reverse(Sval text)
  {
    // This is an easy matter, isn't it?
    return Sval(text.rbegin(), text.rend());
  }

Sval std_string_trim(Sval text, Sopt reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Get the index of the first byte to keep.
    auto start = text.find_first_not_of(rchars);
    if(start == Sval::npos) {
      // There is no byte to keep. Return an empty string.
      return emptyc;
    }
    // Get the index of the last byte to keep.
    auto end = text.find_last_not_of(rchars);
    if((start == 0) && (end == text.size() - 1)) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Return the remaining part of `text`.
    return text.substr(start, end + 1 - start);
  }

Sval std_string_ltrim(Sval text, Sopt reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Get the index of the first byte to keep.
    auto start = text.find_first_not_of(rchars);
    if(start == Sval::npos) {
      // There is no byte to keep. Return an empty string.
      return emptyc;
    }
    if(start == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Return the remaining part of `text`.
    return text.substr(start);
  }

Sval std_string_rtrim(Sval text, Sopt reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Get the index of the last byte to keep.
    auto end = text.find_last_not_of(rchars);
    if(end == Sval::npos) {
      // There is no byte to keep. Return an empty string.
      return emptyc;
    }
    if(end == text.size() - 1) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Return the remaining part of `text`.
    return text.substr(0, end + 1);
  }

Sval std_string_lpad(Sval text, Ival length, Sopt padding)
  {
    Sval res = text;
    auto rpadding = do_get_padding(padding);
    if(length <= 0) {
      // There is nothing to do.
      return res;
    }
    // Fill `rpadding` at the front.
    res.reserve(static_cast<size_t>(length));
    while(res.size() + rpadding.length() <= static_cast<uint64_t>(length)) {
      res.insert(res.end() - text.ssize(), rpadding);
    }
    return res;
  }

Sval std_string_rpad(Sval text, Ival length, Sopt padding)
  {
    Sval res = text;
    auto rpadding = do_get_padding(padding);
    if(length <= 0) {
      // There is nothing to do.
      return res;
    }
    // Fill `rpadding` at the back.
    res.reserve(static_cast<size_t>(length));
    while(res.size() + rpadding.length() <= static_cast<uint64_t>(length)) {
      res.append(rpadding);
    }
    return res;
  }

Sval std_string_to_upper(Sval text)
  {
    // Use reference counting as our advantage.
    Sval res = text;
    char* wptr = nullptr;
    // Translate each character.
    for(size_t i = 0;  i < res.size();  ++i) {
      char c = res[i];
      if((c < 'a') || ('z' < c)) {
        continue;
      }
      // Fork the string as needed.
      if(ROCKET_UNEXPECT(!wptr)) {
        wptr = res.mut_data();
      }
      wptr[i] = static_cast<char>(c - 'a' + 'A');
    }
    return res;
  }

Sval std_string_to_lower(Sval text)
  {
    // Use reference counting as our advantage.
    Sval res = text;
    char* wptr = nullptr;
    // Translate each character.
    for(size_t i = 0;  i < res.size();  ++i) {
      char c = res[i];
      if((c < 'A') || ('Z' < c)) {
        continue;
      }
      // Fork the string as needed.
      if(ROCKET_UNEXPECT(!wptr)) {
        wptr = res.mut_data();
      }
      wptr[i] = static_cast<char>(c - 'A' + 'a');
    }
    return res;
  }

Sval std_string_translate(Sval text, Sval inputs, Sopt outputs)
  {
    // Use reference counting as our advantage.
    Sval res = text;
    char* wptr = nullptr;
    // Translate each character.
    for(size_t i = 0;  i < res.size();  ++i) {
      char c = res[i];
      auto ipos = inputs.find(c);
      if(ipos == Sval::npos) {
        continue;
      }
      // Fork the string as needed.
      if(ROCKET_UNEXPECT(!wptr)) {
        wptr = res.mut_data();
      }
      if(!outputs || (ipos >= outputs->size())) {
        // Erase the byte if there is no replacement.
        // N.B. This must cause no reallocation.
        res.erase(i--, 1);
        continue;
      }
      wptr[i] = outputs->data()[ipos];
    }
    return res;
  }

Aval std_string_explode(Sval text, Sopt delim, Iopt limit)
  {
    uint64_t rlimit = UINT64_MAX;
    if(limit) {
      if(*limit <= 0) {
        ASTERIA_THROW("max number of segments must be positive (limit `$1`)", *limit);
      }
      rlimit = static_cast<uint64_t>(*limit);
    }
    Aval segments;
    if(text.empty()) {
      // Return an empty array.
      return segments;
    }
    if(!delim || delim->empty()) {
      // Split every byte.
      segments.reserve(text.size());
      for(size_t i = 0;  i < text.size();  ++i) {
        size_t b = text[i] & 0xFF;
        // Store a reference to the null-terminated string allocated statically.
        // Don't bother allocating a new buffer of only two characters.
        segments.emplace_back(Sval(::rocket::sref(s_char_table[b], 1)));
      }
      return segments;
    }
    // Break `text` down.
    auto bpos = text.begin();
    auto epos = text.end();
    for(;;) {
      if(segments.size() + 1 >= rlimit) {
        segments.emplace_back(Sval(bpos, epos));
        break;
      }
      auto mpos = ::std::search(bpos, epos, delim->begin(), delim->end());
      if(mpos == epos) {
        segments.emplace_back(Sval(bpos, epos));
        break;
      }
      segments.emplace_back(Sval(bpos, mpos));
      bpos = mpos + delim->ssize();
    }
    return segments;
  }

Sval std_string_implode(Aval segments, Sopt delim)
  {
    Sval text;
    auto nsegs = segments.size();
    if(nsegs == 0) {
      // Return an empty string.
      return text;
    }
    // Append the first string.
    text = segments.front().as_string();
    // Any segment other than the first one follows a delimiter.
    for(size_t i = 1;  i != nsegs;  ++i) {
      if(delim) {
        text += *delim;
      }
      text += segments[i].as_string();
    }
    return text;
  }

Sval std_string_hex_encode(Sval data, Bopt lowercase, Sopt delim)
  {
    Sval text;
    auto rdelim = delim ? ::rocket::sref(*delim) : ::rocket::sref("");
    bool rlowerc = lowercase == true;
    text.reserve(data.size() * (2 + rdelim.length()));
    // These shall be operated in big-endian order.
    uint32_t reg = 0;
    array<char, 2> unit;
    // Encode source data.
    size_t nread = 0;
    while(nread != data.size()) {
      // Read a byte.
      reg = data[nread++] & 0xFF;
      reg <<= 24;
      // Encode it.
      for(size_t i = 0;  i < 2;  ++i) {
        size_t b = ((reg >> 28) * 2 + rlowerc) & 0xFF;
        reg <<= 4;
        unit[i] = s_base16_table[b];
      }
      if(!text.empty()) {
        text.append(rdelim);
      }
      text.append(unit.data(), unit.size());
    }
    return text;
  }

Sopt std_string_hex_decode(Sval text)
  {
    Sval data;
    // These shall be operated in big-endian order.
    uint32_t reg = 0;
    sso_vector<char, 2> unit;
    // Decode source data.
    size_t nread = 0;
    while(nread != text.size()) {
      // Read and identify a character.
      char c = text[nread++];
      const char* pos = do_xstrchr(s_spaces, c);
      if(pos) {
        // The character is a whitespace.
        if(unit.size() != 0) {
          // Fail if it occurs in the middle of a encoding unit.
          return emptyc;
        }
        // Ignore it.
        continue;
      }
      unit.emplace_back(c);
      if(unit.size() != unit.capacity()) {
        // Await remaining digits.
        continue;
      }
      // Decode the current encoding unit if it has been filled up.
      for(size_t i = 0;  i < 2;  ++i) {
        pos = do_xstrchr(s_base16_table, unit[i]);
        if(!pos) {
          // The character is invalid.
          return emptyc;
        }
        auto off = static_cast<size_t>(pos - s_base16_table) / 2;
        ROCKET_ASSERT(off < 16);
        // Accept a digit.
        uint32_t b = off & 0xFF;
        reg <<= 4;
        reg |= b;
      }
      reg <<= 24;
      // Write the unit.
      data.push_back(static_cast<char>(reg >> 24));
      unit.clear();
    }
    if(unit.size() != 0) {
      // Fail in case of excess digits.
      return emptyc;
    }
    return ::rocket::move(data);
  }

Sval std_string_base32_encode(Sval data, Bopt lowercase)
  {
    Sval text;
    bool rlowerc = lowercase == true;
    text.reserve((data.size() + 4) / 5 * 8);
    // These shall be operated in big-endian order.
    uint64_t reg = 0;
    array<char, 8> unit;
    // Encode source data.
    size_t nread = 0;
    while(data.size() - nread >= 5) {
      // Read 5 consecutive bytes.
      for(size_t i = 0;  i < 5;  ++i) {
        uint64_t b = data[nread++] & 0xFF;
        reg <<= 8;
        reg |= b;
      }
      reg <<= 24;
      // Encode them.
      for(size_t i = 0;  i < 8;  ++i) {
        size_t b = ((reg >> 59) * 2 + rlowerc) & 0xFF;
        reg <<= 5;
        unit[i] = s_base32_table[b];
      }
      text.append(unit.data(), unit.size());
    }
    if(nread != data.size()) {
      // Get the start of padding characters.
      auto m = data.size() - nread;
      auto p = (m * 8 + 4) / 5;
      // Read all remaining bytes that cannot fill up a unit.
      for(size_t i = 0;  i < m;  ++i) {
        uint64_t b = data[nread++] & 0xFF;
        reg <<= 8;
        reg |= b;
      }
      reg <<= 64 - m * 8;
      // Encode them.
      for(size_t i = 0;  i < p;  ++i) {
        size_t b = ((reg >> 59) * 2 + rlowerc) & 0xFF;
        reg <<= 5;
        unit[i] = s_base32_table[b];
      }
      for(size_t i = p;  i != 8;  ++i) {
        unit[i] = s_base32_table[64];
      }
      text.append(unit.data(), unit.size());
    }
    return text;
  }

Sopt std_string_base32_decode(Sval text)
  {
    Sval data;
    // These shall be operated in big-endian order.
    uint64_t reg = 0;
    sso_vector<char, 8> unit;
    // Decode source data.
    size_t nread = 0;
    while(nread != text.size()) {
      // Read and identify a character.
      char c = text[nread++];
      const char* pos = do_xstrchr(s_spaces, c);
      if(pos) {
        // The character is a whitespace.
        if(unit.size() != 0) {
          // Fail if it occurs in the middle of a encoding unit.
          return emptyc;
        }
        // Ignore it.
        continue;
      }
      unit.emplace_back(c);
      if(unit.size() != unit.capacity()) {
        // Await remaining digits.
        continue;
      }
      // Get the start of padding characters.
      auto pt = ::std::find(unit.begin(), unit.end(), s_base32_table[64]);
      if(::std::any_of(pt, unit.end(), [&](char cx) { return cx != s_base32_table[64];  })) {
        // Fail if a non-padding character follows a padding character.
        return emptyc;
      }
      auto p = static_cast<size_t>(pt - unit.begin());
      // How many bytes are there in this unit?
      auto m = p * 5 / 8;
      if((m == 0) || ((m * 8 + 4) / 5 != p)) {
        // Fail due to invalid number of non-padding characters.
        return emptyc;
      }
      // Decode the current encoding unit.
      for(size_t i = 0;  i < p;  ++i) {
        pos = do_xstrchr(s_base32_table, unit[i]);
        if(!pos) {
          // The character is invalid.
          return emptyc;
        }
        auto off = static_cast<size_t>(pos - s_base32_table) / 2;
        ROCKET_ASSERT(off < 32);
        // Accept a digit.
        uint64_t b = off & 0xFF;
        reg <<= 5;
        reg |= b;
      }
      reg <<= 64 - p * 5;
      // Write the unit.
      for(size_t i = 0;  i < m;  ++i) {
        data.push_back(static_cast<char>(reg >> 56));
        reg <<= 8;
      }
      unit.clear();
    }
    if(unit.size() != 0) {
      // Fail in case of excess digits.
      return emptyc;
    }
    return ::rocket::move(data);
  }

Sval std_string_base64_encode(Sval data)
  {
    Sval text;
    text.reserve((data.size() + 2) / 3 * 4);
    // These shall be operated in big-endian order.
    uint32_t reg = 0;
    array<char, 4> unit;
    // Encode source data.
    size_t nread = 0;
    while(data.size() - nread >= 3) {
      // Read 3 consecutive bytes.
      for(size_t i = 0;  i < 3;  ++i) {
        uint32_t b = data[nread++] & 0xFF;
        reg <<= 8;
        reg |= b;
      }
      reg <<= 8;
      // Encode them.
      for(size_t i = 0;  i < 4;  ++i) {
        size_t b = (reg >> 26) & 0xFF;
        reg <<= 6;
        unit[i] = s_base64_table[b];
      }
      text.append(unit.data(), unit.size());
    }
    if(nread != data.size()) {
      // Get the start of padding characters.
      auto m = data.size() - nread;
      auto p = (m * 8 + 5) / 6;
      // Read all remaining bytes that cannot fill up a unit.
      for(size_t i = 0;  i < m;  ++i) {
        uint32_t b = data[nread++] & 0xFF;
        reg <<= 8;
        reg |= b;
      }
      reg <<= 32 - m * 8;
      // Encode them.
      for(size_t i = 0;  i < p;  ++i) {
        size_t b = (reg >> 26) & 0xFF;
        reg <<= 6;
        unit[i] = s_base64_table[b];
      }
      for(size_t i = p;  i != 4;  ++i) {
        unit[i] = s_base64_table[64];
      }
      text.append(unit.data(), unit.size());
    }
    return text;
  }

Sopt std_string_base64_decode(Sval text)
  {
    Sval data;
    // These shall be operated in big-endian order.
    uint32_t reg = 0;
    sso_vector<char, 4> unit;
    // Decode source data.
    size_t nread = 0;
    while(nread != text.size()) {
      // Read and identify a character.
      char c = text[nread++];
      const char* pos = do_xstrchr(s_spaces, c);
      if(pos) {
        // The character is a whitespace.
        if(unit.size() != 0) {
          // Fail if it occurs in the middle of a encoding unit.
          return emptyc;
        }
        // Ignore it.
        continue;
      }
      unit.emplace_back(c);
      if(unit.size() != unit.capacity()) {
        // Await remaining digits.
        continue;
      }
      // Get the start of padding characters.
      auto pt = ::std::find(unit.begin(), unit.end(), s_base64_table[64]);
      if(::std::any_of(pt, unit.end(), [&](char cx) { return cx != s_base64_table[64];  })) {
        // Fail if a non-padding character follows a padding character.
        return emptyc;
      }
      auto p = static_cast<size_t>(pt - unit.begin());
      // How many bytes are there in this unit?
      auto m = p * 3 / 4;
      if((m == 0) || ((m * 8 + 5) / 6 != p)) {
        // Fail due to invalid number of non-padding characters.
        return emptyc;
      }
      // Decode the current encoding unit.
      for(size_t i = 0;  i < p;  ++i) {
        pos = do_xstrchr(s_base64_table, unit[i]);
        if(!pos) {
          // The character is invalid.
          return emptyc;
        }
        auto off = static_cast<size_t>(pos - s_base64_table);
        ROCKET_ASSERT(off < 64);
        // Accept a digit.
        uint32_t b = off & 0xFF;
        reg <<= 6;
        reg |= b;
      }
      reg <<= 32 - p * 6;
      // Write the unit.
      for(size_t i = 0;  i < m;  ++i) {
        data.push_back(static_cast<char>(reg >> 24));
        reg <<= 8;
      }
      unit.clear();
    }
    if(unit.size() != 0) {
      // Fail in case of excess digits.
      return emptyc;
    }
    return ::rocket::move(data);
  }

Sopt std_string_utf8_encode(Ival code_point, Bopt permissive)
  {
    Sval text;
    text.reserve(4);
    // Try encoding the code point.
    auto cp = static_cast<char32_t>(::rocket::clamp(code_point, -1, INT32_MAX));
    if(!utf8_encode(text, cp)) {
      // This comparison with `true` is by intention, because it may be unset.
      if(permissive != true) {
        return emptyc;
      }
      // Encode the replacement character.
      utf8_encode(text, 0xFFFD);
    }
    return ::rocket::move(text);
  }

Sopt std_string_utf8_encode(Aval code_points, Bopt permissive)
  {
    Sval text;
    text.reserve(code_points.size() * 3);
    for(const auto& elem : code_points) {
      // Try encoding the code point.
      auto cp = static_cast<char32_t>(::rocket::clamp(elem.as_integer(), -1, INT32_MAX));
      if(!utf8_encode(text, cp)) {
        // This comparison with `true` is by intention, because it may be unset.
        if(permissive != true) {
          return emptyc;
        }
        // Encode the replacement character.
        utf8_encode(text, 0xFFFD);
      }
    }
    return ::rocket::move(text);
  }

Aopt std_string_utf8_decode(Sval text, Bopt permissive)
  {
    Aval code_points;
    code_points.reserve(text.size());
    // Decode code points repeatedly.
    size_t offset = 0;
    for(;;) {
      if(offset >= text.size()) {
        break;
      }
      char32_t cp;
      if(!utf8_decode(cp, text, offset)) {
        // This comparison with `true` is by intention, because it may be unset.
        if(permissive != true) {
          return emptyc;
        }
        // Re-interpret it as an isolated byte.
        cp = text[offset++] & 0xFF;
      }
      code_points.emplace_back(Ival(cp));
    }
    return ::rocket::move(code_points);
  }

Sval std_string_pack_8(Ival value)
  {
    Sval text;
    text.reserve(1);
    do_pack_one_le<int8_t>(text, value);
    return text;
  }

Sval std_string_pack_8(Aval values)
  {
    Sval text;
    text.reserve(values.size());
    for(const auto& elem : values) {
      do_pack_one_le<int8_t>(text, elem.as_integer());
    }
    return text;
  }

Aval std_string_unpack_8(Sval text)
  {
    return do_unpack_le<int8_t>(text);
  }

Sval std_string_pack_16be(Ival value)
  {
    Sval text;
    text.reserve(2);
    do_pack_one_be<int16_t>(text, value);
    return text;
  }

Sval std_string_pack_16be(Aval values)
  {
    Sval text;
    text.reserve(values.size() * 2);
    for(const auto& elem : values) {
      do_pack_one_be<int16_t>(text, elem.as_integer());
    }
    return text;
  }

Aval std_string_unpack_16be(Sval text)
  {
    return do_unpack_be<int16_t>(text);
  }

Sval std_string_pack_16le(Ival value)
  {
    Sval text;
    text.reserve(2);
    do_pack_one_le<int16_t>(text, value);
    return text;
  }

Sval std_string_pack_16le(Aval values)
  {
    Sval text;
    text.reserve(values.size() * 2);
    for(const auto& elem : values) {
      do_pack_one_le<int16_t>(text, elem.as_integer());
    }
    return text;
  }

Aval std_string_unpack_16le(Sval text)
  {
    return do_unpack_le<int16_t>(text);
  }

Sval std_string_pack_32be(Ival value)
  {
    Sval text;
    text.reserve(4);
    do_pack_one_be<int32_t>(text, value);
    return text;
  }

Sval std_string_pack_32be(Aval values)
  {
    Sval text;
    text.reserve(values.size() * 4);
    for(const auto& elem : values) {
      do_pack_one_be<int32_t>(text, elem.as_integer());
    }
    return text;
  }

Aval std_string_unpack_32be(Sval text)
  {
    return do_unpack_be<int32_t>(text);
  }

Sval std_string_pack_32le(Ival value)
  {
    Sval text;
    text.reserve(4);
    do_pack_one_le<int32_t>(text, value);
    return text;
  }

Sval std_string_pack_32le(Aval values)
  {
    Sval text;
    text.reserve(values.size() * 4);
    for(const auto& elem : values) {
      do_pack_one_le<int32_t>(text, elem.as_integer());
    }
    return text;
  }

Aval std_string_unpack_32le(Sval text)
  {
    return do_unpack_le<int32_t>(text);
  }

Sval std_string_pack_64be(Ival value)
  {
    Sval text;
    text.reserve(8);
    do_pack_one_be<int64_t>(text, value);
    return text;
  }

Sval std_string_pack_64be(Aval values)
  {
    Sval text;
    text.reserve(values.size() * 8);
    for(const auto& elem : values) {
      do_pack_one_be<int64_t>(text, elem.as_integer());
    }
    return text;
  }

Aval std_string_unpack_64be(Sval text)
  {
    return do_unpack_be<int64_t>(text);
  }

Sval std_string_pack_64le(Ival value)
  {
    Sval text;
    text.reserve(8);
    do_pack_one_le<int64_t>(text, value);
    return text;
  }

Sval std_string_pack_64le(Aval values)
  {
    Sval text;
    text.reserve(values.size() * 8);
    for(const auto& elem : values) {
      do_pack_one_le<int64_t>(text, elem.as_integer());
    }
    return text;
  }

Aval std_string_unpack_64le(Sval text)
  {
    return do_unpack_le<int64_t>(text);
  }

Sval std_string_format(Sval templ, cow_vector<Value> values)
  {
    // Prepare inserters.
    cow_vector<::rocket::formatter> insts;
    insts.reserve(values.size());
    for(size_t i = 0;  i < values.size();  ++i) {
      insts.push_back({
        [](tinyfmt& fmt, const void* ptr) -> tinyfmt& { return static_cast<const Value*>(ptr)->print(fmt);  },
        values.data() + i
      });
    }
    // Compose the string into a stream.
    ::rocket::tinyfmt_str fmt;
    vformat(fmt, templ.data(), templ.size(), insts.data(), insts.size());
    return fmt.extract_string();
  }

void create_bindings_string(Oval& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.string.slice()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("slice"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.slice(text, from, [length])`\n"
          "\n"
          "  * Copies a subrange of `text` to create a new byte string. Bytes\n"
          "    are copied from `from` if it is non-negative, or from\n"
          "    `lengthof(text) + from` otherwise. If `length` is set to an\n"
          "    `integer`, no more than this number of bytes will be copied. If\n"
          "    it is absent, all bytes from `from` to the end of `text` will\n"
          "    be copied. If `from` is outside `text`, an empty `string` is\n"
          "    returned.\n"
          "\n"
          "  * Returns the specified substring of `text`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.slice"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Ival from;
          Iopt length;
          if(reader.I().g(text).g(from).g(length).F()) {
            // Call the binding function.
            return std_string_slice(::rocket::move(text), ::rocket::move(from), ::rocket::move(length));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.replace_slice()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("replace_slice"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.replace_slice(text, from, replacement)`\n"
          "\n"
          "  * Replaces all bytes from `from` to the end of `text` with\n"
          "    `replacement` and returns the new byte string. If `from` is\n"
          "    negative, it specifies an offset from the end of `text`. This\n"
          "    function returns a new `string` without modifying `text`.\n"
          "\n"
          "  * Returns a `string` with the subrange replaced.\n"
          "\n"
          "`std.string.replace_slice(text, from, [length], replacement)`\n"
          "\n"
          "  * Replaces a subrange of `text` with `replacement` to create a\n"
          "    new byte string. `from` specifies the start of the subrange to\n"
          "    replace. If `from` is negative, it specifies an offset from the\n"
          "    end of `text`. `length` specifies the maximum number of bytes\n"
          "    to replace. If it is set to `null`, this function is equivalent\n"
          "    to `replace_slice(text, from, replacement)`. This function\n"
          "    returns a new `string` without modifying `text`.\n"
          "\n"
          "  * Returns a `string` with the subrange replaced.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.replace"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          Sval text;
          Ival from;
          Sval replacement;
          if(reader.I().g(text).g(from).S(state).g(replacement).F()) {
            // Call the binding function.
            return std_string_replace_slice(::rocket::move(text), ::rocket::move(from), ::rocket::move(replacement));
          }
          Iopt length;
          if(reader.L(state).g(length).g(replacement).F()) {
            // Call the binding function.
            return std_string_replace_slice(::rocket::move(text), ::rocket::move(from), ::rocket::move(length),
                                            ::rocket::move(replacement));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.compare()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("compare"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.compare(text1, text2, [length])`\n"
          "\n"
          "  * Performs lexicographical comparison on two byte strings. If\n"
          "    `length` is set to an `integer`, no more than this number of\n"
          "    bytes are compared. This function behaves like the `strncmp()`\n"
          "    function in C, except that null characters do not terminate\n"
          "    strings.\n"
          "\n"
          "  * Returns a positive `integer` if `text1` compares greater than\n"
          "    `text2`, a negative `integer` if `text1` compares less than\n"
          "    `text2`, or zero if `text1` compares equal to `text2`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.compare"), ::rocket::ref(args));
          // Parse arguments.
          Sval text1;
          Sval text2;
          Iopt length;
          if(reader.I().g(text1).g(text2).g(length).F()) {
            // Call the binding function.
            return std_string_compare(::rocket::move(text1), ::rocket::move(text2), ::rocket::move(length));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.starts_with()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("starts_with"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.starts_with(text, prefix)`\n"
          "\n"
          "  * Checks whether `prefix` is a prefix of `text`. The empty\n"
          "    `string` is considered to be a prefix of any string.\n"
          "\n"
          "  * Returns `true` if `prefix` is a prefix of `text`, or `false`\n"
          "    otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.starts_with"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Sval prefix;
          if(reader.I().g(text).g(prefix).F()) {
            // Call the binding function.
            return std_string_starts_with(::rocket::move(text), ::rocket::move(prefix));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.ends_with()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("ends_with"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.ends_with(text, suffix)`\n"
          "\n"
          "  * Checks whether `suffix` is a suffix of `text`. The empty\n"
          "    `string` is considered to be a suffix of any string.\n"
          "\n"
          "  * Returns `true` if `suffix` is a suffix of `text`, or `false`\n"
          "    otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.ends_with"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Sval suffix;
          if(reader.I().g(text).g(suffix).F()) {
            // Call the binding function.
            return std_string_ends_with(::rocket::move(text), ::rocket::move(suffix));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.find()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.find(text, pattern)`\n"
          "\n"
          "  * Searches `text` for the first occurrence of `pattern`.\n"
          "\n"
          "  * Returns the subscript of the first byte of the first match of\n"
          "    `pattern` in `text` if one is found, which is always\n"
          "    non-negative, or `null` otherwise.\n"
          "\n"
          "`std.string.find(text, from, pattern)`\n"
          "\n"
          "  * Searches `text` for the first occurrence of `pattern`. The\n"
          "    search operation is performed on the same subrange that would\n"
          "    be returned by `slice(text, from)`.\n"
          "\n"
          "  * Returns the subscript of the first byte of the first match of\n"
          "    `pattern` in `text` if one is found, which is always\n"
          "    non-negative, or `null` otherwise.\n"
          "\n"
          "`std.string.find(text, from, [length], pattern)`\n"
          "\n"
          "  * Searches `text` for the first occurrence of `pattern`. The\n"
          "    search operation is performed on the same subrange that would\n"
          "    be returned by `slice(text, from, length)`.\n"
          "\n"
          "  * Returns the subscript of the first byte of the first match of\n"
          "    `pattern` in `text` if one is found, which is always\n"
          "    non-negative, or `null` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.find"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          Sval text;
          Sval pattern;
          if(reader.I().g(text).S(state).g(pattern).F()) {
            // Call the binding function.
            return std_string_find(::rocket::move(text), ::rocket::move(pattern));
          }
          Ival from;
          if(reader.L(state).g(from).S(state).g(pattern).F()) {
            // Call the binding function.
            return std_string_find(::rocket::move(text), ::rocket::move(from), ::rocket::move(pattern));
          }
          Iopt length;
          if(reader.L(state).g(length).g(pattern).F()) {
            // Call the binding function.
            return std_string_find(::rocket::move(text), ::rocket::move(from), ::rocket::move(length),
                                   ::rocket::move(pattern));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.rfind()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.rfind(text, pattern)`\n"
          "\n"
          "  * Searches `text` for the last occurrence of `pattern`.\n"
          "\n"
          "  * Returns the subscript of the first byte of the last match of\n"
          "    `pattern` in `text` if one is found, which is always\n"
          "    non-negative, or `null` otherwise.\n"
          "\n"
          "`std.string.rfind(text, from, pattern)`\n"
          "\n"
          "  * Searches `text` for the last occurrence of `pattern`. The\n"
          "    search operation is performed on the same subrange that would\n"
          "    be returned by `slice(text, from)`.\n"
          "\n"
          "  * Returns the subscript of the first byte of the last match of\n"
          "    `pattern` in `text` if one is found, which is always\n"
          "    non-negative, or `null` otherwise.\n"
          "\n"
          "`std.string.rfind(text, from, [length], pattern)`\n"
          "\n"
          "  * Searches `text` for the last occurrence of `pattern`.\n"
          "\n"
          "  * Returns the subscript of the first byte of the last match of\n"
          "    `pattern` in `text` if one is found, which is always\n"
          "    non-negative, or `null` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.rfind"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          Sval text;
          Sval pattern;
          if(reader.I().g(text).S(state).g(pattern).F()) {
            // Call the binding function.
            return std_string_rfind(::rocket::move(text), ::rocket::move(pattern));
          }
          Ival from;
          if(reader.L(state).g(from).S(state).g(pattern).F()) {
            // Call the binding function.
            return std_string_rfind(::rocket::move(text), ::rocket::move(from), ::rocket::move(pattern));
          }
          Iopt length;
          if(reader.L(state).g(length).g(pattern).F()) {
            // Call the binding function.
            return std_string_rfind(::rocket::move(text), ::rocket::move(from), ::rocket::move(length),
                                    ::rocket::move(pattern));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.find_and_replace()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_and_replace"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.find_and_replace(text, pattern, replacement)`\n"
          "\n"
          "  * Searches `text` and replaces all occurrences of `pattern` with\n"
          "    `replacement`. This function returns a new `string` without\n"
          "    modifying `text`.\n"
          "\n"
          "  * Returns the string with `pattern` replaced. If `text` does not\n"
          "    contain `pattern`, it is returned intact.\n"
          "\n"
          "`std.string.find_and_replace(text, from, pattern, replacement)`\n"
          "\n"
          "  * Searches `text` and replaces all occurrences of `pattern` with\n"
          "    `replacement`. The search operation is performed on the same\n"
          "    subrange that would be returned by `slice(text, from)`. This\n"
          "    function returns a new `string` without modifying `text`.\n"
          "\n"
          "  * Returns the string with `pattern` replaced. If `text` does not\n"
          "    contain `pattern`, it is returned intact.\n"
          "\n"
          "`std.string.find_and_replace(text, from, [length], pattern, replacement)`\n"
          "\n"
          "  * Searches `text` and replaces all occurrences of `pattern` with\n"
          "    `replacement`. The search operation is performed on the same\n"
          "    subrange that would be returned by `slice(text, from, length)`.\n"
          "    This function returns a new `string` without modifying `text`.\n"
          "\n"
          "  * Returns the string with `pattern` replaced. If `text` does not\n"
          "    contain `pattern`, it is returned intact.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.find_and_replace"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          Sval text;
          Sval pattern;
          Sval replacement;
          if(reader.I().g(text).S(state).g(pattern).g(replacement).F()) {
            // Call the binding function.
            return std_string_find_and_replace(::rocket::move(text), ::rocket::move(pattern),
                                               ::rocket::move(replacement));
          }
          Ival from;
          if(reader.L(state).g(from).S(state).g(pattern).g(replacement).F()) {
            // Call the binding function.
            return std_string_find_and_replace(::rocket::move(text), ::rocket::move(from), ::rocket::move(pattern),
                                               ::rocket::move(replacement));
          }
          Iopt length;
          if(reader.L(state).g(length).g(pattern).g(replacement).F()) {
            // Call the binding function.
            return std_string_find_and_replace(::rocket::move(text), ::rocket::move(from), ::rocket::move(length),
                                               ::rocket::move(pattern), ::rocket::move(replacement));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.find_any_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_any_of"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.find_any_of(text, accept)`\n"
          "\n"
          "  * Searches `text` for bytes that exist in `accept`.\n"
          "\n"
          "  * Returns the subscript of the first byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
          "\n"
          "`std.string.find_any_of(text, from, accept)`\n"
          "\n"
          "  * Searches `text` for bytes that exist in `accept`. The search\n"
          "    operation is performed on the same subrange that would be\n"
          "    returned by `slice(text, from)`.\n"
          "\n"
          "  * Returns the subscript of the first byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
          "\n"
          "`std.string.find_any_of(text, from, [length], accept)`\n"
          "\n"
          "  * Searches `text` for bytes that exist in `accept`. The search\n"
          "    operation is performed on the same subrange that would be\n"
          "    returned by `slice(text, from, length)`.\n"
          "\n"
          "  * Returns the subscript of the first byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.find_any_of"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          Sval text;
          Sval accept;
          if(reader.I().g(text).S(state).g(accept).F()) {
            // Call the binding function.
            return std_string_find_any_of(::rocket::move(text), ::rocket::move(accept));
          }
          Ival from;
          if(reader.L(state).g(from).S(state).g(accept).F()) {
            // Call the binding function.
            return std_string_find_any_of(::rocket::move(text), ::rocket::move(from), ::rocket::move(accept));
          }
          Iopt length;
          if(reader.L(state).g(length).g(accept).F()) {
            // Call the binding function.
            return std_string_find_any_of(::rocket::move(text), ::rocket::move(from), ::rocket::move(length),
                                          ::rocket::move(accept));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.rfind_any_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind_any_of"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.rfind_any_of(text, accept)`\n"
          "\n"
          "  * Searches `text` for bytes that exist in `accept`.\n"
          "\n"
          "  * Returns the subscript of the last byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
          "\n"
          "`std.string.rfind_any_of(text, from, accept)`\n"
          "\n"
          "  * Searches `text` for bytes that exist in `accept`. The search\n"
          "    operation is performed on the same subrange that would be\n"
          "    returned by `slice(text, from)`.\n"
          "\n"
          "  * Returns the subscript of the last byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
          "\n"
          "`std.string.rfind_any_of(text, from, [length], accept)`\n"
          "\n"
          "  * Searches `text` for bytes that exist in `accept`. The search\n"
          "    operation is performed on the same subrange that would be\n"
          "    returned by `slice(text, from, length)`.\n"
          "\n"
          "  * Returns the subscript of the last byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.rfind_any_of"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          Sval text;
          Sval accept;
          if(reader.I().g(text).S(state).g(accept).F()) {
            // Call the binding function.
            return std_string_rfind_any_of(::rocket::move(text), ::rocket::move(accept));
          }
          Ival from;
          if(reader.L(state).g(from).S(state).g(accept).F()) {
            // Call the binding function.
            return std_string_rfind_any_of(::rocket::move(text), ::rocket::move(from), ::rocket::move(accept));
          }
          Iopt length;
          if(reader.L(state).g(length).g(accept).F()) {
            // Call the binding function.
            return std_string_rfind_any_of(::rocket::move(text), ::rocket::move(from), ::rocket::move(length),
                                           ::rocket::move(accept));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.find_not_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_not_of"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.find_not_of(text, reject)`\n"
          "\n"
          "  * Searches `text` for bytes that does not exist in `reject`.\n"
          "\n"
          "  * Returns the subscript of the first byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
          "\n"
          "`std.string.find_not_of(text, from, reject)`\n"
          "\n"
          "  * Searches `text` for bytes that does not exist in `reject`. The\n"
          "    search operation is performed on the same subrange that would\n"
          "    be returned by `slice(text, from)`.\n"
          "\n"
          "  * Returns the subscript of the first byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
          "\n"
          "`std.string.find_not_of(text, from, [length], reject)`\n"
          "\n"
          "  * Searches `text` for bytes that does not exist in `reject`. The\n"
          "    search operation is performed on the same subrange that would\n"
          "    be returned by `slice(text, from, length)`.\n"
          "\n"
          "  * Returns the subscript of the first byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.find_not_of"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          Sval text;
          Sval accept;
          if(reader.I().g(text).S(state).g(accept).F()) {
            // Call the binding function.
            return std_string_find_not_of(::rocket::move(text), ::rocket::move(accept));
          }
          Ival from;
          if(reader.L(state).g(from).S(state).g(accept).F()) {
            // Call the binding function.
            return std_string_find_not_of(::rocket::move(text), ::rocket::move(from), ::rocket::move(accept));
          }
          Iopt length;
          if(reader.L(state).g(length).g(accept).F()) {
            // Call the binding function.
            return std_string_find_not_of(::rocket::move(text), ::rocket::move(from), ::rocket::move(length),
                                          ::rocket::move(accept));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.rfind_not_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind_not_of"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.rfind_not_of(text, reject)`\n"
          "\n"
          "  * Searches `text` for bytes that does not exist in `reject`.\n"
          "\n"
          "  * Returns the subscript of the last byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
          "\n"
          "`std.string.rfind_not_of(text, from, reject)`\n"
          "\n"
          "  * Searches `text` for bytes that does not exist in `reject`. The\n"
          "    search operation is performed on the same subrange that would\n"
          "    be returned by `slice(text, from)`.\n"
          "\n"
          "  * Returns the subscript of the last byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
          "\n"
          "`std.string.rfind_not_of(text, from, [length], reject)`\n"
          "\n"
          "  * Searches `text` for bytes that does not exist in `reject`. The\n"
          "    search operation is performed on the same subrange that would\n"
          "    be returned by `slice(text, from, length)`.\n"
          "\n"
          "  * Returns the subscript of the last byte found, which is always\n"
          "    non-negative; or `null` if no such byte exists.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.rfind_not_of"), ::rocket::ref(args));
          Argument_Reader::State state;
          // Parse arguments.
          Sval text;
          Sval accept;
          if(reader.I().g(text).S(state).g(accept).F()) {
            // Call the binding function.
            return std_string_rfind_not_of(::rocket::move(text), ::rocket::move(accept));
          }
          Ival from;
          if(reader.L(state).g(from).S(state).g(accept).F()) {
            // Call the binding function.
            return std_string_rfind_not_of(::rocket::move(text), ::rocket::move(from), ::rocket::move(accept));
          }
          Iopt length;
          if(reader.L(state).g(length).g(accept).F()) {
            // Call the binding function.
            return std_string_rfind_not_of(::rocket::move(text), ::rocket::move(from), ::rocket::move(length),
                                           ::rocket::move(accept));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.reverse()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("reverse"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.reverse(text)`\n"
          "\n"
          "  * Reverses a byte string. This function returns a new `string`\n"
          "    without modifying `text`.\n"
          "\n"
          "  * Returns the reversed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.reverse"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_reverse(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.trim()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("trim"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.trim(text, [reject])`\n"
          "\n"
          "  * Removes the longest prefix and suffix consisting solely bytes\n"
          "    from `reject`. If `reject` is empty, no byte is removed. If\n"
          "    `reject` is not specified, spaces and tabs are removed. This\n"
          "    function returns a new `string` without modifying `text`.\n"
          "\n"
          "  * Returns the trimmed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.trim"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Sopt reject;
          if(reader.I().g(text).g(reject).F()) {
            // Call the binding function.
            return std_string_trim(::rocket::move(text), ::rocket::move(reject));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.ltrim()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("ltrim"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.ltrim(text, [reject])`\n"
          "\n"
          "  * Removes the longest prefix consisting solely bytes from\n"
          "    `reject`. If `reject` is empty, no byte is removed. If `reject`\n"
          "    is not specified, spaces and tabs are removed. This function\n"
          "    returns a new `string` without modifying `text`.\n"
          "\n"
          "  * Returns the trimmed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.ltrim"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Sopt reject;
          if(reader.I().g(text).g(reject).F()) {
            // Call the binding function.
            return std_string_ltrim(::rocket::move(text), ::rocket::move(reject));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.rtrim()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rtrim"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.rtrim(text, [reject])`\n"
          "\n"
          "  * Removes the longest suffix consisting solely bytes from\n"
          "    `reject`. If `reject` is empty, no byte is removed. If `reject`\n"
          "    is not specified, spaces and tabs are removed. This function\n"
          "    returns a new `string` without modifying `text`.\n"
          "\n"
          "  * Returns the trimmed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.rtrim"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Sopt reject;
          if(reader.I().g(text).g(reject).F()) {
            // Call the binding function.
            return std_string_rtrim(::rocket::move(text), ::rocket::move(reject));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.lpad()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("lpad"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.lpad(text, length, [padding])`\n"
          "\n"
          "  * Prepends `text` with `padding` repeatedly, until its length\n"
          "    would exceed `length`. The default value of `padding` is a\n"
          "    `string` consisting of a space. This function returns a new\n"
          "    `string` without modifying `text`.\n"
          "\n"
          "  * Returns the padded string.\n"
          "\n"
          "  * Throws an exception if `padding` is empty.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.lpad"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Ival length;
          Sopt padding;
          if(reader.I().g(text).g(length).g(padding).F()) {
            // Call the binding function.
            return std_string_lpad(::rocket::move(text), ::rocket::move(length), ::rocket::move(padding));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.rpad()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rpad"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.rpad(text, length, [padding])`\n"
          "\n"
          "  * Appends `text` with `padding` repeatedly, until its length\n"
          "    would exceed `length`. The default value of `padding` is a\n"
          "    `string` consisting of a space. This function returns a new\n"
          "    `string` without modifying `text`.\n"
          "\n"
          "  * Returns the padded string.\n"
          "\n"
          "  * Throws an exception if `padding` is empty.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.rpad"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Ival length;
          Sopt padding;
          if(reader.I().g(text).g(length).g(padding).F()) {
            // Call the binding function.
            return std_string_rpad(::rocket::move(text), ::rocket::move(length), ::rocket::move(padding));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.to_upper()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("to_upper"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.to_upper(text)`\n"
          "\n"
          "  * Converts all lowercase English letters in `text` to their\n"
          "    uppercase counterparts. This function returns a new `string`\n"
          "    without modifying `text`.\n"
          "\n"
          "  * Returns a new `string` after the conversion.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.to_upper"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_to_upper(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.to_lower()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("to_lower"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.to_lower(text)`\n"
          "\n"
          "  * Converts all lowercase English letters in `text` to their\n"
          "    uppercase counterparts. This function returns a new `string`\n"
          "    without modifying `text`.\n"
          "\n"
          "  * Returns a new `string` after the conversion.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.to_lower"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_to_lower(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.translate()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("translate"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.translate(text, inputs, [outputs])`\n"
          "\n"
          "  * Performs bytewise translation on the given string. For every\n"
          "    byte in `text` that is also found in `inputs`, if there is a\n"
          "    corresponding replacement byte in `outputs` with the same\n"
          "    subscript, it is replaced with the latter; if no replacement\n"
          "    exists, because `outputs` is shorter than `inputs` or is null,\n"
          "    it is deleted. If `outputs` is longer than `inputs`, excess\n"
          "    bytes are ignored. Bytes that do not exist in `inputs` are left\n"
          "    intact. This function returns a new `string` without modifying\n"
          "    `text`.\n"
          "\n"
          "  * Returns the translated `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.translate"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Sval inputs;
          Sopt outputs;
          if(reader.I().g(text).g(inputs).g(outputs).F()) {
            // Call the binding function.
            return std_string_translate(::rocket::move(text), ::rocket::move(inputs), ::rocket::move(outputs));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.explode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("explode"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.explode(text, [delim], [limit])`\n"
          "\n"
          "  * Breaks `text` down into segments, separated by `delim`. If\n"
          "    `delim` is `null` or an empty `string`, every byte becomes a\n"
          "    segment. If `limit` is set to a positive `integer`, there will\n"
          "    be no more segments than this number; the last segment will\n"
          "    contain all the remaining bytes of the `text`.\n"
          "\n"
          "  * Returns an `array` containing the broken-down segments. If\n"
          "    `text` is empty, an empty `array` is returned.\n"
          "\n"
          "  * Throws an exception if `limit` is negative or zero.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.explode"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Sopt delim;
          Iopt limit;
          if(reader.I().g(text).g(delim).g(limit).F()) {
            // Call the binding function.
            return std_string_explode(::rocket::move(text), ::rocket::move(delim), ::rocket::move(limit));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.implode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("implode"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.implode(segments, [delim])`\n"
          "\n"
          "  * Concatenates elements of an array, `segments`, to create a new\n"
          "    `string`. All segments shall be `string`s. If `delim` is\n"
          "    specified, it is inserted between adjacent segments.\n"
          "\n"
          "  * Returns a `string` containing all segments. If `segments` is\n"
          "    empty, an empty `string` is returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.implode"), ::rocket::ref(args));
          // Parse arguments.
          Aval segments;
          Sopt delim;
          if(reader.I().g(segments).g(delim).F()) {
            // Call the binding function.
            return std_string_implode(::rocket::move(segments), ::rocket::move(delim));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.hex_encode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("hex_encode"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.hex_encode(data, [lowercase], [delim])`\n"
          "\n"
          "  * Encodes all bytes in `data` as 2-digit hexadecimal numbers and\n"
          "    concatenates them. If `lowercase` is set to `true`, hexadecimal\n"
          "    digits above `9` are encoded as `abcdef`; otherwise they are\n"
          "    encoded as `ABCDEF`. If `delim` is specified, it is inserted\n"
          "    between adjacent bytes.\n"
          "\n"
          "  * Returns the encoded `string`. If `data` is empty, an empty\n"
          "    `string` is returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.hex_encode"), ::rocket::ref(args));
          // Parse arguments.
          Sval data;
          Bopt lowercase;
          Sopt delim;
          if(reader.I().g(data).g(lowercase).g(delim).F()) {
            // Call the binding function.
            return std_string_hex_encode(::rocket::move(data), ::rocket::move(lowercase), ::rocket::move(delim));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.hex_decode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("hex_decode"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.hex_decode(text)`\n"
          "\n"
          "  * Decodes all hexadecimal digits from `text` and converts them to\n"
          "    bytes. Whitespaces can be used to delimit bytes; they shall not\n"
          "    occur between digits in the same byte. Consequently, the total\n"
          "    number of non-whitespace characters must be a multiple of two.\n"
          "    Invalid characters cause parse errors.\n"
          "\n"
          "  * Returns a `string` containing decoded bytes. If `text` is empty\n"
          "    or consists of only whitespaces, an empty `string` is returned.\n"
          "    In the case of parse errors, `null` is returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.hex_decode"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_hex_decode(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.base32_encode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("base32_encode"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.base32_encode(text, [uppercase])`\n"
          "\n"
          "  * Encodes all bytes in `text` according to the base32 encoding\n"
          "    specified by IETF RFC 4648. If `uppercase` is set to `true`,\n"
          "    uppercase letters are used to represent values through `0` to\n"
          "    `25`; otherwise, lowercase letters are used. The length of\n"
          "    encoded data is always a multiple of 8; padding characters\n"
          "    are mandatory.\n"
          "\n"
          "  * Returns the encoded `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.base32_encode"), ::rocket::ref(args));
          // Parse arguments.
          Sval data;
          Bopt lowercase;
          if(reader.I().g(data).g(lowercase).F()) {
            // Call the binding function.
            return std_string_base32_encode(::rocket::move(data), ::rocket::move(lowercase));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.base32_decode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("base32_decode"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.base32_decode(dstr)`\n"
          "\n"
          "  * Decodes data encoded in base32, as specified by IETF RFC 4648.\n"
          "    Whitespaces can be used to delimit encoding units; they shall\n"
          "    not occur between characters in the same unit. Consequently,\n"
          "    the number of non-whitespace characters must be a multiple of\n"
          "    eight. Invalid characters cause parse errors.\n"
          "\n"
          "  * Returns a `string` containing decoded bytes. If `dstr` is empty\n"
          "    or consists of only whitespaces, an empty `string` is returned.\n"
          "    In the case of parse errors, `null` is returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.base32_decode"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_base32_decode(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.base64_encode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("base64_encode"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.base64_encode(text)`\n"
          "\n"
          "  * Encodes all bytes in `text` according to the base64 encoding\n"
          "    specified by IETF RFC 4648. The length of encoded data is\n"
          "    always a multiple of 4; padding characters are mandatory.\n"
          "\n"
          "  * Returns the encoded `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.base64_encode"), ::rocket::ref(args));
          // Parse arguments.
          Sval data;
          if(reader.I().g(data).F()) {
            // Call the binding function.
            return std_string_base64_encode(::rocket::move(data));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.base64_decode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("base64_decode"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.base64_decode(dstr)`\n"
          "\n"
          "  * Decodes data encoded in base64, as specified by IETF RFC 4648.\n"
          "    Whitespaces can be used to delimit encoding units; they shall\n"
          "    not occur between characters in the same unit. Consequently,\n"
          "    the number of non-whitespace characters must be a multiple of\n"
          "    four. Invalid characters cause parse errors.\n"
          "    Decodes base64-encoded data. Whitespaces can be used to\n"
          "\n"
          "  * Returns a `string` containing decoded bytes. If `dstr` is empty\n"
          "    or consists of only whitespaces, an empty `string` is returned.\n"
          "    In the case of parse errors, `null` is returned.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.base64_decode"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_base64_decode(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.utf8_encode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utf8_encode"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.utf8_encode(code_points, [permissive])`\n"
          "\n"
          "  * Encodes code points from `code_points` into an UTF-8 `string`.\n"
          "  `code_points` can be either an `integer` or an `array` of\n"
          "  `integer`s. When an invalid code point is encountered, if\n"
          "  `permissive` is set to `true`, it is replaced with the\n"
          "  replacement character `\"\\uFFFD\"` and consequently encoded as\n"
          "  `\"\\xEF\\xBF\\xBD\"`; otherwise this function fails.\n"
          "\n"
          "  * Returns the encoded `string` on success, or `null` otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.utf8_encode"), ::rocket::ref(args));
          // Parse arguments.
          Ival code_point;
          Bopt permissive;
          if(reader.I().g(code_point).g(permissive).F()) {
            // Call the binding function.
            return std_string_utf8_encode(::rocket::move(code_point), ::rocket::move(permissive));
          }
          Aval code_points;
          if(reader.I().g(code_points).g(permissive).F()) {
            // Call the binding function.
            return std_string_utf8_encode(::rocket::move(code_points), ::rocket::move(permissive));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.utf8_decode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utf8_decode"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.utf8_decode(text, [permissive])`\n"
          "\n"
          "  * Decodes `text`, which is expected to be a `string` containing\n"
          "    UTF-8 code units, into an `array` of code points, represented\n"
          "    as `integer`s. When an invalid code sequence is encountered, if\n"
          "    `permissive` is set to `true`, all code units of it are\n"
          "    re-interpreted as isolated bytes according to ISO/IEC 8859-1;\n"
          "    otherwise this function fails.\n"
          "\n"
          "  * Returns an `array` containing decoded code points, or `null`\n"
          "    otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.utf8_decode"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          Bopt permissive;
          if(reader.I().g(text).g(permissive).F()) {
            // Call the binding function.
            return std_string_utf8_decode(::rocket::move(text), ::rocket::move(permissive));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.pack_8()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_8"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.pack_8(values)`\n"
          "\n"
          "  * Packs a series of 8-bit integers into a `string`. `values` can\n"
          "    be either an `integer` or an `array` of `integer`s, all of\n"
          "    which are truncated to 8 bits then copied into a `string`.\n"
          "\n"
          "  * Returns the packed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.pack_8"), ::rocket::ref(args));
          // Parse arguments.
          Ival value;
          if(reader.I().g(value).F()) {
            // Call the binding function.
            return std_string_pack_8(::rocket::move(value));
          }
          Aval values;
          if(reader.I().g(values).F()) {
            // Call the binding function.
            return std_string_pack_8(::rocket::move(values));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.unpack_8()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_8"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.unpack_8(text)`\n"
          "\n"
          "  * Unpacks 8-bit integers from a `string`. The contents of `text`\n"
          "    are re-interpreted as contiguous signed 8-bit integers, all of\n"
          "    which are sign-extended to 64 bits then copied into an `array`.\n"
          "\n"
          "  * Returns an `array` containing unpacked integers.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.unpack_8"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_unpack_8(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.pack_16be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_16be"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.pack_16be(values)`\n"
          "\n"
          "  * Packs a series of 16-bit integers into a `string`. `values` can\n"
          "    be either an `integer` or an `array` of `integers`, all of\n"
          "    which are truncated to 16 bits then copied into a `string` in\n"
          "    the big-endian byte order.\n"
          "\n"
          "  * Returns the packed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.pack_16be"), ::rocket::ref(args));
          // Parse arguments.
          Ival value;
          if(reader.I().g(value).F()) {
            // Call the binding function.
            return std_string_pack_16be(::rocket::move(value));
          }
          Aval values;
          if(reader.I().g(values).F()) {
            // Call the binding function.
            return std_string_pack_16be(::rocket::move(values));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.unpack_16be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_16be"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.unpack_16be(text)`\n"
          "\n"
          "  * Unpacks 16-bit integers from a `string`. The contents of `text`\n"
          "    are re-interpreted as contiguous signed 16-bit integers in the\n"
          "    big-endian byte order, all of which are sign-extended to 64\n"
          "    bits then copied into an `array`.\n"
          "\n"
          "  * Returns an `array` containing unpacked integers.\n"
          "\n"
          "  * Throws an exception if the length of `text` is not a multiple\n"
          "    of 2.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.unpack_16be"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_unpack_16be(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.pack_16le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_16le"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.pack_16le(values)`\n"
          "\n"
          "  * Packs a series of 16-bit integers into a `string`. `values` can\n"
          "    be either an `integer` or an `array` of `integers`, all of\n"
          "    which are truncated to 16 bits then copied into a `string` in\n"
          "    the little-endian byte order.\n"
          "\n"
          "  * Returns the packed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.pack_16le"), ::rocket::ref(args));
          // Parse arguments.
          Ival value;
          if(reader.I().g(value).F()) {
            // Call the binding function.
            return std_string_pack_16le(::rocket::move(value));
          }
          Aval values;
          if(reader.I().g(values).F()) {
            // Call the binding function.
            return std_string_pack_16le(::rocket::move(values));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.unpack_16le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_16le"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.unpack_16le(text)`\n"
          "\n"
          "  * Unpacks 16-bit integers from a `string`. The contents of `text`\n"
          "    are re-interpreted as contiguous signed 16-bit integers in the\n"
          "    little-endian byte order, all of which are sign-extended to 64\n"
          "    bits then copied into an `array`.\n"
          "\n"
          "  * Returns an `array` containing unpacked integers.\n"
          "\n"
          "  * Throws an exception if the length of `text` is not a multiple\n"
          "    of 2.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.unpack_16le"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_unpack_16le(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.pack_32be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_32be"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.pack_32be(values)`\n"
          "\n"
          "  * Packs a series of 32-bit integers into a `string`. `values` can\n"
          "    be either an `integer` or an `array` of `integers`, all of\n"
          "    which are truncated to 32 bits then copied into a `string` in\n"
          "    the big-endian byte order.\n"
          "\n"
          "  * Returns the packed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.pack_32be"), ::rocket::ref(args));
          // Parse arguments.
          Ival value;
          if(reader.I().g(value).F()) {
            // Call the binding function.
            return std_string_pack_32be(::rocket::move(value));
          }
          Aval values;
          if(reader.I().g(values).F()) {
            // Call the binding function.
            return std_string_pack_32be(::rocket::move(values));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.unpack_32be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_32be"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.unpack_32be(text)`\n"
          "\n"
          "  * Unpacks 32-bit integers from a `string`. The contents of `text`\n"
          "    are re-interpreted as contiguous signed 32-bit integers in the\n"
          "    big-endian byte order, all of which are sign-extended to 64\n"
          "    bits then copied into an `array`.\n"
          "\n"
          "  * Returns an `array` containing unpacked integers.\n"
          "\n"
          "  * Throws an exception if the length of `text` is not a multiple\n"
          "    of 4.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.unpack_32be"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_unpack_32be(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.pack_32le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_32le"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.pack_32le(values)`\n"
          "\n"
          "  * Packs a series of 32-bit integers into a `string`. `values` can\n"
          "    be either an `integer` or an `array` of `integers`, all of\n"
          "    which are truncated to 32 bits then copied into a `string` in\n"
          "    the little-endian byte order.\n"
          "\n"
          "  * Returns the packed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.pack_32le"), ::rocket::ref(args));
          // Parse arguments.
          Ival value;
          if(reader.I().g(value).F()) {
            // Call the binding function.
            return std_string_pack_32le(::rocket::move(value));
          }
          Aval values;
          if(reader.I().g(values).F()) {
            // Call the binding function.
            return std_string_pack_32le(::rocket::move(values));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.unpack_32le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_32le"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.unpack_32le(text)`\n"
          "\n"
          "  * Unpacks 32-bit integers from a `string`. The contents of `text`\n"
          "    are re-interpreted as contiguous signed 32-bit integers in the\n"
          "    little-endian byte order, all of which are sign-extended to 64\n"
          "    bits then copied into an `array`.\n"
          "\n"
          "  * Returns an `array` containing unpacked integers.\n"
          "\n"
          "  * Throws an exception if the length of `text` is not a multiple\n"
          "    of 4.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.unpack_32le"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_unpack_32le(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.pack_64be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_64be"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.pack_64be(values)`\n"
          "\n"
          "  * Packs a series of 64-bit integers into a `string`. `values` can\n"
          "    be either an `integer` or an `array` of `integers`, all of\n"
          "    which are copied into a `string` in the big-endian byte order.\n"
          "\n"
          "  * Returns the packed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.pack_64be"), ::rocket::ref(args));
          // Parse arguments.
          Ival value;
          if(reader.I().g(value).F()) {
            // Call the binding function.
            return std_string_pack_64be(::rocket::move(value));
          }
          Aval values;
          if(reader.I().g(values).F()) {
            // Call the binding function.
            return std_string_pack_64be(::rocket::move(values));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.unpack_64be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_64be"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.unpack_64be(text)`\n"
          "\n"
          "  * Unpacks 64-bit integers from a `string`. The contents of `text`\n"
          "    are re-interpreted as contiguous signed 64-bit integers in the\n"
          "    big-endian byte order, all of which are copied into an `array`.\n"
          "\n"
          "  * Returns an `array` containing unpacked integers.\n"
          "\n"
          "  * Throws an exception if the length of `text` is not a multiple\n"
          "    of 8.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.unpack_64be"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_unpack_64be(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.pack_64le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_64le"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.pack_64le(values)`\n"
          "\n"
          "  * Packs a series of 64-bit integers into a `string`. `values` can\n"
          "    be either an `integer` or an `array` of `integers`, all of\n"
          "    which are copied into a `string` in the little-endian byte\n"
          "    order.\n"
          "\n"
          "  * Returns the packed `string`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.pack_64le"), ::rocket::ref(args));
          // Parse arguments.
          Ival value;
          if(reader.I().g(value).F()) {
            // Call the binding function.
            return std_string_pack_64le(::rocket::move(value));
          }
          Aval values;
          if(reader.I().g(values).F()) {
            // Call the binding function.
            return std_string_pack_64le(::rocket::move(values));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.unpack_64le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_64le"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.unpack_64le(text)`\n"
          "\n"
          "  * Unpacks 64-bit integers from a `string`. The contents of `text`\n"
          "    are re-interpreted as contiguous signed 64-bit integers in the\n"
          "    little-endian byte order, all of which are copied into an\n"
          "    `array`.\n"
          "\n"
          "  * Returns an `array` containing unpacked integers.\n"
          "\n"
          "  * Throws an exception if the length of `text` is not a multiple\n"
          "    of 8.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.unpack_64le"), ::rocket::ref(args));
          // Parse arguments.
          Sval text;
          if(reader.I().g(text).F()) {
            // Call the binding function.
            return std_string_unpack_64le(::rocket::move(text));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.string.format()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("format"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.string.format(templ, ...)`\n"
          "\n"
          " * Compose a `string` according to the template string `templ`, as\n"
          "   follows:\n"
          "\n"
          "   * A sequence of `$$` is replaced with a literal `$`.\n"
          "   * A sequence of `${NNN}`, where `NNN` is at most three decimal\n"
          "     numerals, is replaced with the NNN-th argument. If `NNN` is\n"
          "     zero, it is replaced with `templ` itself.\n"
          "   * A sequence of `$N`, where `N` is a single decimal numeral,\n"
          "     behaves the same as `${N}`.\n"
          "   * All other characters are copied verbatim.\n"
          "\n"
          " * Returns the composed `string`.\n"
          "\n"
          " * Throws an exception if `templ` contains invalid placeholder\n"
          "   sequences, or when a placeholder sequence has no corresponding\n"
          "   argument.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value {
          Argument_Reader reader(::rocket::sref("std.string.format"), ::rocket::ref(args));
          // Parse arguments.
          Sval templ;
          cow_vector<Value> values;
          if(reader.I().g(templ).F(values)) {
            // Call the binding function.
            return std_string_format(::rocket::move(templ), ::rocket::move(values));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // End of `std.string`
    //===================================================================
  }

}  // namespace Asteria
