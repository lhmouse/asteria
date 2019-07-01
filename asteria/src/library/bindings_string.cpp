// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_string.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"
#include <bitset>

namespace Asteria {

    namespace {

    Pair<G_string::const_iterator, G_string::const_iterator> do_slice(const G_string& text, G_string::const_iterator tbegin, const Opt<G_integer>& length)
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

    Pair<G_string::const_iterator, G_string::const_iterator> do_slice(const G_string& text, const G_integer& from, const Opt<G_integer>& length)
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

G_string std_string_slice(const G_string& text, const G_integer& from, const Opt<G_integer>& length)
  {
    auto range = do_slice(text, from, length);
    if((range.first == text.begin()) && (range.second == text.end())) {
      // Use reference counting as our advantage.
      return text;
    }
    return G_string(range.first, range.second);
  }

G_string std_string_replace_slice(const G_string& text, const G_integer& from, const G_string& replacement)
  {
    G_string res = text;
    auto range = do_slice(res, from, rocket::nullopt);
    // Replace the subrange.
    res.replace(range.first, range.second, replacement);
    return res;
  }

G_string std_string_replace_slice(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& replacement)
  {
    G_string res = text;
    auto range = do_slice(res, from, length);
    // Replace the subrange.
    res.replace(range.first, range.second, replacement);
    return res;
  }

G_integer std_string_compare(const G_string& text1, const G_string& text2, const Opt<G_integer>& length)
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
    return text1.compare(0, static_cast<std::size_t>(*length), text2, 0, static_cast<std::size_t>(*length));
  }

G_boolean std_string_starts_with(const G_string& text, const G_string& prefix)
  {
    return text.starts_with(prefix);
  }

G_boolean std_string_ends_with(const G_string& text, const G_string& suffix)
  {
    return text.ends_with(suffix);
  }

    namespace {

    // https://en.wikipedia.org/wiki/Boyer-Moore-Horspool_algorithm
    template<typename IteratorT> Opt<IteratorT> do_find_opt(IteratorT tbegin, IteratorT tend, IteratorT pbegin, IteratorT pend)
      {
        auto plen = std::distance(pbegin, pend);
        if(plen <= 0) {
          // Return a match at the the beginning if the pattern is empty.
          return tbegin;
        }
        // Build a table according to the Bad Character Rule.
        Array<std::ptrdiff_t, 0x100> bcr_table;
        for(std::size_t i = 0; i != 0x100; ++i) {
          bcr_table[i] = plen;
        }
        for(std::ptrdiff_t i = plen - 1; i != 0; --i) {
          bcr_table[pend[~i] & 0xFF] = i;
        }
        // Search for the pattern.
        auto tpos = tbegin;
        for(;;) {
          if(tend - tpos < plen) {
            return rocket::nullopt;
          }
          if(std::equal(pbegin, pend, tpos)) {
            break;
          }
          tpos += bcr_table[tpos[plen - 1] & 0xFF];
        }
        return rocket::move(tpos);
      }

    }

Opt<G_integer> std_string_find(const G_string& text, const G_string& pattern)
  {
    auto range = std::make_pair(text.begin(), text.end());
    auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - text.begin();
  }

Opt<G_integer> std_string_find(const G_string& text, const G_integer& from, const G_string& pattern)
  {
    auto range = do_slice(text, from, rocket::nullopt);
    auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - text.begin();
  }

Opt<G_integer> std_string_find(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& pattern)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - text.begin();
  }

Opt<G_integer> std_string_rfind(const G_string& text, const G_string& pattern)
  {
    auto range = std::make_pair(text.begin(), text.end());
    auto qit = do_find_opt(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), pattern.rbegin(), pattern.rend());
    if(!qit) {
      return rocket::nullopt;
    }
    return text.rend() - *qit - pattern.ssize();
  }

Opt<G_integer> std_string_rfind(const G_string& text, const G_integer& from, const G_string& pattern)
  {
    auto range = do_slice(text, from, rocket::nullopt);
    auto qit = do_find_opt(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), pattern.rbegin(), pattern.rend());
    if(!qit) {
      return rocket::nullopt;
    }
    return text.rend() - *qit - pattern.ssize();
  }

Opt<G_integer> std_string_rfind(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& pattern)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_opt(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), pattern.rbegin(), pattern.rend());
    if(!qit) {
      return rocket::nullopt;
    }
    return text.rend() - *qit - pattern.ssize();
  }

G_string std_string_find_and_replace(const G_string& text, const G_string& pattern, const G_string& replacement)
  {
    G_string res = text;
    auto range = std::make_pair(res.begin(), res.end());
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
      if(!qit) {
        // Make use of reference counting if no match has been found.
        break;
      }
      // Save offsets before replacing due to possibility of invalidation of iterators.
      auto roffs = std::make_pair(*qit - res.begin(), range.second - res.begin());
      res.replace(*qit, *qit + pattern.ssize(), replacement);
      range = std::make_pair(res.begin() + roffs.first + replacement.ssize(), res.begin() + roffs.second);
    }
    return res;
  }

G_string std_string_find_and_replace(const G_string& text, const G_integer& from, const G_string& pattern, const G_string& replacement)
  {
    G_string res = text;
    auto range = do_slice(res, from, rocket::nullopt);
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
      if(!qit) {
        // Make use of reference counting if no match has been found.
        break;
      }
      // Save offsets before replacing due to possibility of invalidation of iterators.
      auto roffs = std::make_pair(*qit - res.begin(), range.second - res.begin());
      res.replace(*qit, *qit + pattern.ssize(), replacement);
      range = std::make_pair(res.begin() + roffs.first + replacement.ssize(), res.begin() + roffs.second);
    }
    return res;
  }

G_string std_string_find_and_replace(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& pattern, const G_string& replacement)
  {
    G_string res = text;
    auto range = do_slice(res, from, length);
    for(;;) {
      auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
      if(!qit) {
        // Make use of reference counting if no match has been found.
        break;
      }
      // Save offsets before replacing due to possibility of invalidation of iterators.
      auto roffs = std::make_pair(*qit - res.begin(), range.second - res.begin());
      res.replace(*qit, *qit + pattern.ssize(), replacement);
      range = std::make_pair(res.begin() + roffs.first + replacement.ssize(), res.begin() + roffs.second);
    }
    return res;
  }

    namespace {

    template<typename IteratorT> Opt<IteratorT> do_find_of_opt(IteratorT begin, IteratorT end, const G_string& set, bool match)
      {
        // Make a lookup table.
        std::bitset<256> table;
        for(auto it = set.begin(); it != set.end(); ++it) {
          table.set(*it & 0xFF);
        }
        // Search the range.
        for(auto it = rocket::move(begin); it != end; ++it) {
          if(table.test(*it & 0xFF) == match) {
            return rocket::move(it);
          }
        }
        return rocket::nullopt;
      }

    }

Opt<G_integer> std_string_find_any_of(const G_string& text, const G_string& accept)
  {
    auto qit = do_find_of_opt(text.begin(), text.end(), accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - text.begin();
  }

Opt<G_integer> std_string_find_any_of(const G_string& text, const G_integer& from, const G_string& accept)
  {
    auto range = do_slice(text, from, rocket::nullopt);
    auto qit = do_find_of_opt(range.first, range.second, accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - text.begin();
  }

Opt<G_integer> std_string_find_any_of(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& accept)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(range.first, range.second, accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - text.begin();
  }

Opt<G_integer> std_string_find_not_of(const G_string& text, const G_string& reject)
  {
    auto qit = do_find_of_opt(text.begin(), text.end(), reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - text.begin();
  }

Opt<G_integer> std_string_find_not_of(const G_string& text, const G_integer& from, const G_string& reject)
  {
    auto range = do_slice(text, from, rocket::nullopt);
    auto qit = do_find_of_opt(range.first, range.second, reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - text.begin();
  }

Opt<G_integer> std_string_find_not_of(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& reject)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(range.first, range.second, reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return *qit - text.begin();
  }

Opt<G_integer> std_string_rfind_any_of(const G_string& text, const G_string& accept)
  {
    auto qit = do_find_of_opt(text.rbegin(), text.rend(), accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return text.rend() - *qit - 1;
  }

Opt<G_integer> std_string_rfind_any_of(const G_string& text, const G_integer& from, const G_string& accept)
  {
    auto range = do_slice(text, from, rocket::nullopt);
    auto qit = do_find_of_opt(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return text.rend() - *qit - 1;
  }

Opt<G_integer> std_string_rfind_any_of(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& accept)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return text.rend() - *qit - 1;
  }

Opt<G_integer> std_string_rfind_not_of(const G_string& text, const G_string& reject)
  {
    auto qit = do_find_of_opt(text.rbegin(), text.rend(), reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return text.rend() - *qit - 1;
  }

Opt<G_integer> std_string_rfind_not_of(const G_string& text, const G_integer& from, const G_string& reject)
  {
    auto range = do_slice(text, from, rocket::nullopt);
    auto qit = do_find_of_opt(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return text.rend() - *qit - 1;
  }

Opt<G_integer> std_string_rfind_not_of(const G_string& text, const G_integer& from, const Opt<G_integer>& length, const G_string& reject)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return text.rend() - *qit - 1;
  }

G_string std_string_reverse(const G_string& text)
  {
    // This is an easy matter, isn't it?
    return G_string(text.rbegin(), text.rend());
  }

    namespace {

    inline G_string::shallow_type do_get_reject(const Opt<G_string>& reject)
      {
        if(!reject) {
          return rocket::sref(" \t");
        }
        return rocket::sref(*reject);
      }

    }

G_string std_string_trim(const G_string& text, const Opt<G_string>& reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Get the index of the first byte to keep.
    auto start = text.find_first_not_of(rchars);
    if(start == G_string::npos) {
      // There is no byte to keep. Return an empty string.
      return rocket::clear;
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

G_string std_string_ltrim(const G_string& text, const Opt<G_string>& reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Get the index of the first byte to keep.
    auto start = text.find_first_not_of(rchars);
    if(start == G_string::npos) {
      // There is no byte to keep. Return an empty string.
      return rocket::clear;
    }
    if(start == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Return the remaining part of `text`.
    return text.substr(start);
  }

G_string std_string_rtrim(const G_string& text, const Opt<G_string>& reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Get the index of the last byte to keep.
    auto end = text.find_last_not_of(rchars);
    if(end == G_string::npos) {
      // There is no byte to keep. Return an empty string.
      return rocket::clear;
    }
    if(end == text.size() - 1) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Return the remaining part of `text`.
    return text.substr(0, end + 1);
  }

    namespace {

    inline G_string::shallow_type do_get_padding(const Opt<G_string>& padding)
      {
        if(!padding) {
          return rocket::sref(" ");
        }
        if(padding->empty()) {
          ASTERIA_THROW_RUNTIME_ERROR("Padding empty strings could result in infinite loops.");
        }
        return rocket::sref(*padding);
      }

    }

G_string std_string_lpad(const G_string& text, const G_integer& length, const Opt<G_string>& padding)
  {
    G_string res = text;
    auto rpadding = do_get_padding(padding);
    if(length <= 0) {
      // There is nothing to do.
      return res;
    }
    // Fill `rpadding` at the front.
    res.reserve(static_cast<std::size_t>(length));
    while(res.size() + rpadding.length() <= static_cast<std::uint64_t>(length)) {
      res.insert(res.end() - text.ssize(), rpadding);
    }
    return res;
  }

G_string std_string_rpad(const G_string& text, const G_integer& length, const Opt<G_string>& padding)
  {
    G_string res = text;
    auto rpadding = do_get_padding(padding);
    if(length <= 0) {
      // There is nothing to do.
      return res;
    }
    // Fill `rpadding` at the back.
    res.reserve(static_cast<std::size_t>(length));
    while(res.size() + rpadding.length() <= static_cast<std::uint64_t>(length)) {
      res.append(rpadding);
    }
    return res;
  }

G_string std_string_to_upper(const G_string& text)
  {
    // Use reference counting as our advantage.
    G_string res = text;
    char* wptr = nullptr;
    // Translate each character.
    for(std::size_t i = 0; i < res.size(); ++i) {
      char ch = res[i];
      if((ch < 'a') || ('z' < ch)) {
        continue;
      }
      // Fork the string as needed.
      if(ROCKET_UNEXPECT(!wptr)) {
        wptr = res.mut_data();
      }
      wptr[i] = static_cast<char>(ch - 'a' + 'A');
    }
    return res;
  }

G_string std_string_to_lower(const G_string& text)
  {
    // Use reference counting as our advantage.
    G_string res = text;
    char* wptr = nullptr;
    // Translate each character.
    for(std::size_t i = 0; i < res.size(); ++i) {
      char ch = res[i];
      if((ch < 'A') || ('Z' < ch)) {
        continue;
      }
      // Fork the string as needed.
      if(ROCKET_UNEXPECT(!wptr)) {
        wptr = res.mut_data();
      }
      wptr[i] = static_cast<char>(ch - 'A' + 'a');
    }
    return res;
  }

G_string std_string_translate(const G_string& text, const G_string& inputs, const Opt<G_string>& outputs)
  {
    // Use reference counting as our advantage.
    G_string res = text;
    char* wptr = nullptr;
    // Translate each character.
    for(std::size_t i = 0; i < res.size(); ++i) {
      char ch = res[i];
      auto ipos = inputs.find(ch);
      if(ipos == G_string::npos) {
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

    namespace {

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

    }

G_array std_string_explode(const G_string& text, const Opt<G_string>& delim, const Opt<G_integer>& limit)
  {
    std::uint64_t rlimit = UINT64_MAX;
    if(limit) {
      if(*limit <= 0) {
        ASTERIA_THROW_RUNTIME_ERROR("The limit of number of segments must be greater than zero (got `", *limit, "`).");
      }
      rlimit = static_cast<std::uint64_t>(*limit);
    }
    G_array segments;
    if(text.empty()) {
      // Return an empty array.
      return segments;
    }
    if(!delim || delim->empty()) {
      // Split every byte.
      segments.reserve(text.size());
      for(char ch : text) {
        segments.emplace_back(G_string(rocket::sref(s_char_table[ch & 0xFF], 1)));
      }
      return segments;
    }
    // Break `text` down.
    std::size_t bpos = 0;
    std::size_t epos;
    for(;;) {
      if(segments.size() + 1 >= rlimit) {
        segments.emplace_back(G_string(text, bpos));
        break;
      }
      epos = text.find(*delim, bpos);
      if(epos == G_string::npos) {
        segments.emplace_back(G_string(text, bpos));
        break;
      }
      segments.emplace_back(G_string(text, bpos, epos - bpos));
      bpos = epos + delim->size();
    }
    return segments;
  }

G_string std_string_implode(const G_array& segments, const Opt<G_string>& delim)
  {
    G_string text;
    auto nsegs = segments.size();
    if(nsegs == 0) {
      // Return an empty string.
      return text;
    }
    // Append the first string.
    text = segments.front().as_string();
    // Any segment other than the first one follows a delimiter.
    for(std::size_t i = 1; i != nsegs; ++i) {
      if(delim) {
        text += *delim;
      }
      text += segments[i].as_string();
    }
    return text;
  }

    namespace {

    constexpr char s_base16_table[] = "00112233445566778899AaBbCcDdEeFf \f\n\r\t\v";

    template<std::size_t sizeT> constexpr const char* do_slitchr(const char (&str)[sizeT], char ch) noexcept
      {
        return std::find(str, str + sizeT - 1, ch);
      }

    }

G_string std_string_hex_encode(const G_string& data, const Opt<G_boolean>& lowercase, const Opt<G_string>& delim)
  {
    G_string text;
    auto ntotal = data.size();
    if(ntotal == 0) {
      // Return an empty string.
      return text;
    }
    bool rlowerc = lowercase == true;
    Array<char, 2> unit;
    // Reserve storage for alphabets.
    text.reserve(ntotal * unit.size());
    std::size_t nread = 0;
    while(nread != ntotal) {
      // Read a byte in big-endian order.
      std::uint32_t reg = static_cast<std::uint32_t>(data[nread++] & 0xFF) << 24;
      // Encode it.
      for(std::size_t i = 0; i != 2; ++i) {
        unit[i] = s_base16_table[((reg >> (27 - i * 4)) & 0x1E) + rlowerc];
      }
      if(!text.empty() && delim) {
        text.append(*delim);
      }
      text.append(unit.data(), unit.size());
    }
    return text;
  }

Opt<G_string> std_string_hex_decode(const G_string& text)
  {
    G_string data;
    // Remember the value of a previous digit. `-1` means no such digit exists.
    int dprev = -1;
    auto flush = [&]()
      {
        if(dprev == -1) {
          return;
        }
        data.push_back(static_cast<char>(dprev));
        dprev = -1;
      };
    // Decode characters one by one.
    for(char ch : text) {
      // Identify this character.
      auto pos = do_slitchr(s_base16_table, ch);
      if(*pos == 0) {
        // Fail due to an invalid character.
        return rocket::nullopt;
      }
      // The first 32 characters are hex digits. The others are spaces.
      auto dcur = static_cast<std::uint8_t>((pos - s_base16_table) / 2);
      if(dcur >= 16) {
        // Ignore space characters. But if we have had a digit, flush it.
        flush();
        continue;
      }
      if(dprev == -1) {
        // Save this digit.
        dprev = dcur;
        continue;
      }
      // We have got two digits now.
      // Make a byte and write it.
      data.push_back(static_cast<char>(dprev * 16 + dcur));
      dprev = -1;
    }
    // If we have had a digit, flush it.
    flush();
    return rocket::move(data);
  }

Opt<G_string> std_string_utf8_encode(const G_integer& code_point, const Opt<G_boolean>& permissive)
  {
    G_string text;
    text.reserve(4);
    // Try encoding the code point.
    auto cp = static_cast<char32_t>(rocket::clamp(code_point, -1, INT32_MAX));
    if(!utf8_encode(text, cp)) {
      // This comparison with `true` is by intention, because it may be unset.
      if(permissive != true) {
        return rocket::nullopt;
      }
      // Encode the replacement character.
      utf8_encode(text, 0xFFFD);
    }
    return rocket::move(text);
  }

Opt<G_string> std_string_utf8_encode(const G_array& code_points, const Opt<G_boolean>& permissive)
  {
    G_string text;
    text.reserve(code_points.size() * 3);
    for(const auto& elem : code_points) {
      // Try encoding the code point.
      auto cp = static_cast<char32_t>(rocket::clamp(elem.as_integer(), -1, INT32_MAX));
      if(!utf8_encode(text, cp)) {
        // This comparison with `true` is by intention, because it may be unset.
        if(permissive != true) {
          return rocket::nullopt;
        }
        // Encode the replacement character.
        utf8_encode(text, 0xFFFD);
      }
    }
    return rocket::move(text);
  }

Opt<G_array> std_string_utf8_decode(const G_string& text, const Opt<G_boolean>& permissive)
  {
    G_array code_points;
    code_points.reserve(text.size());
    // Decode code points repeatedly.
    std::size_t offset = 0;
    for(;;) {
      if(offset >= text.size()) {
        break;
      }
      char32_t cp;
      if(!utf8_decode(cp, text, offset)) {
        // This comparison with `true` is by intention, because it may be unset.
        if(permissive != true) {
          return rocket::nullopt;
        }
        // Re-interpret it as an isolated byte.
        cp = text[offset++] & 0xFF;
      }
      code_points.emplace_back(G_integer(cp));
    }
    return rocket::move(code_points);
  }

    namespace {

    template<bool bigendT, typename WordT> bool do_pack_one_impl(G_string& text, const G_integer& value)
      {
        // Define temporary storage.
        Array<char, sizeof(WordT)> stor_le;
        std::uint64_t word = static_cast<std::uint64_t>(value);
        // Write it in little-endian order.
        for(auto& byte : stor_le) {
          byte = static_cast<char>(word);
          word >>= 8;
        }
        // Append this word.
        if(bigendT) {
          text.append(stor_le.rbegin(), stor_le.rend());
        } else {
          text.append(stor_le.begin(), stor_le.end());
        }
        return true;
      }
    template<typename WordT> bool do_pack_one_be(G_string& text, const G_integer& value)
      {
        return do_pack_one_impl<1, WordT>(text, value);
      }
    template<typename WordT> bool do_pack_one_le(G_string& text, const G_integer& value)
      {
        return do_pack_one_impl<0, WordT>(text, value);
      }

    template<bool bigendT, typename WordT> G_array do_unpack_impl(const G_string& text)
      {
        G_array values;
        // Define temporary storage.
        Array<char, sizeof(WordT)> stor_be;
        std::uint64_t word = 0;
        // How many words will the result have?
        auto nwords = text.size() / stor_be.size();
        if(text.size() != nwords * stor_be.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("The length of the source string must be a multiple of `", stor_be.size(), "` (got `", text.size(), "`).");
        }
        values.reserve(nwords);
        // Unpack integers.
        for(std::size_t i = 0; i < nwords; ++i) {
          // Read some bytes in big-endian order.
          if(bigendT) {
            std::copy_n(text.data() + i * stor_be.size(), stor_be.size(), stor_be.begin());
          } else {
            std::copy_n(text.data() + i * stor_be.size(), stor_be.size(), stor_be.rbegin());
          }
          // Assemble the word.
          for(const auto& byte : stor_be) {
            word <<= 8;
            word |= static_cast<unsigned char>(byte);
          }
          // Append the word.
          values.emplace_back(G_integer(static_cast<WordT>(word)));
        }
        return values;
      }
    template<typename WordT> G_array do_unpack_be(const G_string& text)
      {
        return do_unpack_impl<1, WordT>(text);
      }
    template<typename WordT> G_array do_unpack_le(const G_string& text)
      {
        return do_unpack_impl<0, WordT>(text);
      }

    }

G_string std_string_pack8(const G_integer& value)
  {
    G_string text;
    text.reserve(1);
    do_pack_one_le<std::int8_t>(text, value);
    return text;
  }

G_string std_string_pack8(const G_array& values)
  {
    G_string text;
    text.reserve(values.size());
    for(const auto& elem : values) {
      do_pack_one_le<std::int8_t>(text, elem.as_integer());
    }
    return text;
  }

G_array std_string_unpack8(const G_string& text)
  {
    return do_unpack_le<std::int8_t>(text);
  }

G_string std_string_pack_16be(const G_integer& value)
  {
    G_string text;
    text.reserve(2);
    do_pack_one_be<std::int16_t>(text, value);
    return text;
  }

G_string std_string_pack_16be(const G_array& values)
  {
    G_string text;
    text.reserve(values.size() * 2);
    for(const auto& elem : values) {
      do_pack_one_be<std::int16_t>(text, elem.as_integer());
    }
    return text;
  }

G_array std_string_unpack_16be(const G_string& text)
  {
    return do_unpack_be<std::int16_t>(text);
  }

G_string std_string_pack_16le(const G_integer& value)
  {
    G_string text;
    text.reserve(2);
    do_pack_one_le<std::int16_t>(text, value);
    return text;
  }

G_string std_string_pack_16le(const G_array& values)
  {
    G_string text;
    text.reserve(values.size() * 2);
    for(const auto& elem : values) {
      do_pack_one_le<std::int16_t>(text, elem.as_integer());
    }
    return text;
  }

G_array std_string_unpack_16le(const G_string& text)
  {
    return do_unpack_le<std::int16_t>(text);
  }

G_string std_string_pack_32be(const G_integer& value)
  {
    G_string text;
    text.reserve(4);
    do_pack_one_be<std::int32_t>(text, value);
    return text;
  }

G_string std_string_pack_32be(const G_array& values)
  {
    G_string text;
    text.reserve(values.size() * 4);
    for(const auto& elem : values) {
      do_pack_one_be<std::int32_t>(text, elem.as_integer());
    }
    return text;
  }

G_array std_string_unpack_32be(const G_string& text)
  {
    return do_unpack_be<std::int32_t>(text);
  }

G_string std_string_pack_32le(const G_integer& value)
  {
    G_string text;
    text.reserve(4);
    do_pack_one_le<std::int32_t>(text, value);
    return text;
  }

G_string std_string_pack_32le(const G_array& values)
  {
    G_string text;
    text.reserve(values.size() * 4);
    for(const auto& elem : values) {
      do_pack_one_le<std::int32_t>(text, elem.as_integer());
    }
    return text;
  }

G_array std_string_unpack_32le(const G_string& text)
  {
    return do_unpack_le<std::int32_t>(text);
  }

G_string std_string_pack_64be(const G_integer& value)
  {
    G_string text;
    text.reserve(8);
    do_pack_one_be<std::int64_t>(text, value);
    return text;
  }

G_string std_string_pack_64be(const G_array& values)
  {
    G_string text;
    text.reserve(values.size() * 8);
    for(const auto& elem : values) {
      do_pack_one_be<std::int64_t>(text, elem.as_integer());
    }
    return text;
  }

G_array std_string_unpack_64be(const G_string& text)
  {
    return do_unpack_be<std::int64_t>(text);
  }

G_string std_string_pack_64le(const G_integer& value)
  {
    G_string text;
    text.reserve(8);
    do_pack_one_le<std::int64_t>(text, value);
    return text;
  }

G_string std_string_pack_64le(const G_array& values)
  {
    G_string text;
    text.reserve(values.size() * 8);
    for(const auto& elem : values) {
      do_pack_one_le<std::int64_t>(text, elem.as_integer());
    }
    return text;
  }

G_array std_string_unpack_64le(const G_string& text)
  {
    return do_unpack_le<std::int64_t>(text);
  }

void create_bindings_string(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.string.slice()`
    //===================================================================
    result.insert_or_assign(rocket::sref("slice"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.slice"), args);
            // Parse arguments.
            G_string text;
            G_integer from;
            Opt<G_integer> length;
            if(reader.start().g(text).g(from).g(length).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_slice(text, from, length) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.replace_slice()`
    //===================================================================
    result.insert_or_assign(rocket::sref("replace_slice"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.replace"), args);
            Argument_Reader::State state;
            // Parse arguments.
            G_string text;
            G_integer from;
            G_string replacement;
            if(reader.start().g(text).g(from).save(state).g(replacement).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_replace_slice(text, from, replacement) };
              return rocket::move(xref);
            }
            Opt<G_integer> length;
            if(reader.load(state).g(length).g(replacement).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_replace_slice(text, from, length, replacement) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.compare()`
    //===================================================================
    result.insert_or_assign(rocket::sref("compare"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.compare"), args);
            // Parse arguments.
            G_string text1;
            G_string text2;
            Opt<G_integer> length;
            if(reader.start().g(text1).g(text2).g(length).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_compare(text1, text2, length) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.starts_with()`
    //===================================================================
    result.insert_or_assign(rocket::sref("starts_with"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.string.starts_with(text, prefix)`\n"
            "\n"
            "  * Checks whether `prefix` is a prefix of `text`. The empty\n"
            "    `string` is considered to be a prefix of any string.\n"
            "\n"
            "  * Returns `true` if `prefix` is a prefix of `text`, or `false`\n"
            "    otherwise.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.starts_with"), args);
            // Parse arguments.
            G_string text;
            G_string prefix;
            if(reader.start().g(text).g(prefix).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_starts_with(text, prefix) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.ends_with()`
    //===================================================================
    result.insert_or_assign(rocket::sref("ends_with"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.string.ends_with(text, suffix)`\n"
            "\n"
            "  * Checks whether `suffix` is a suffix of `text`. The empty\n"
            "    `string` is considered to be a suffix of any string.\n"
            "\n"
            "  * Returns `true` if `suffix` is a suffix of `text`, or `false`\n"
            "    otherwise.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.ends_with"), args);
            // Parse arguments.
            G_string text;
            G_string suffix;
            if(reader.start().g(text).g(suffix).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_ends_with(text, suffix) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.find()`
    //===================================================================
    result.insert_or_assign(rocket::sref("find"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.find"), args);
            Argument_Reader::State state;
            // Parse arguments.
            G_string text;
            G_string pattern;
            if(reader.start().g(text).save(state).g(pattern).finish()) {
              // Call the binding function.
              auto qindex = std_string_find(text, pattern);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            G_integer from;
            if(reader.load(state).g(from).save(state).g(pattern).finish()) {
              // Call the binding function.
              auto qindex = std_string_find(text, from, pattern);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<G_integer> length;
            if(reader.load(state).g(length).g(pattern).finish()) {
              // Call the binding function.
              auto qindex = std_string_find(text, from, length, pattern);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.rfind()`
    //===================================================================
    result.insert_or_assign(rocket::sref("rfind"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.rfind"), args);
            Argument_Reader::State state;
            // Parse arguments.
            G_string text;
            G_string pattern;
            if(reader.start().g(text).save(state).g(pattern).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind(text, pattern);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            G_integer from;
            if(reader.load(state).g(from).save(state).g(pattern).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind(text, from, pattern);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<G_integer> length;
            if(reader.load(state).g(length).g(pattern).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind(text, from, length, pattern);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.find_and_replace()`
    //===================================================================
    result.insert_or_assign(rocket::sref("find_and_replace"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.find_and_replace"), args);
            Argument_Reader::State state;
            // Parse arguments.
            G_string text;
            G_string pattern;
            G_string replacement;
            if(reader.start().g(text).save(state).g(pattern).g(replacement).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_find_and_replace(text, pattern, replacement) };
              return rocket::move(xref);
            }
            G_integer from;
            if(reader.load(state).g(from).save(state).g(pattern).g(replacement).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_find_and_replace(text, from, pattern, replacement) };
              return rocket::move(xref);
            }
            Opt<G_integer> length;
            if(reader.load(state).g(length).g(pattern).g(replacement).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_find_and_replace(text, from, length, pattern, replacement) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.find_any_of()`
    //===================================================================
    result.insert_or_assign(rocket::sref("find_any_of"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.find_any_of"), args);
            Argument_Reader::State state;
            // Parse arguments.
            G_string text;
            G_string accept;
            if(reader.start().g(text).save(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_find_any_of(text, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            G_integer from;
            if(reader.load(state).g(from).save(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_find_any_of(text, from, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<G_integer> length;
            if(reader.load(state).g(length).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_find_any_of(text, from, length, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.rfind_any_of()`
    //===================================================================
    result.insert_or_assign(rocket::sref("rfind_any_of"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.rfind_any_of"), args);
            Argument_Reader::State state;
            // Parse arguments.
            G_string text;
            G_string accept;
            if(reader.start().g(text).save(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind_any_of(text, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            G_integer from;
            if(reader.load(state).g(from).save(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind_any_of(text, from, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<G_integer> length;
            if(reader.load(state).g(length).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind_any_of(text, from, length, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.find_not_of()`
    //===================================================================
    result.insert_or_assign(rocket::sref("find_not_of"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.find_not_of"), args);
            Argument_Reader::State state;
            // Parse arguments.
            G_string text;
            G_string accept;
            if(reader.start().g(text).save(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_find_not_of(text, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            G_integer from;
            if(reader.load(state).g(from).save(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_find_not_of(text, from, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<G_integer> length;
            if(reader.load(state).g(length).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_find_not_of(text, from, length, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.rfind_not_of()`
    //===================================================================
    result.insert_or_assign(rocket::sref("rfind_not_of"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.rfind_not_of"), args);
            Argument_Reader::State state;
            // Parse arguments.
            G_string text;
            G_string accept;
            if(reader.start().g(text).save(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind_not_of(text, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            G_integer from;
            if(reader.load(state).g(from).save(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind_not_of(text, from, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<G_integer> length;
            if(reader.load(state).g(length).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind_not_of(text, from, length, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.reverse()`
    //===================================================================
    result.insert_or_assign(rocket::sref("reverse"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.string.reverse(text)`\n"
            "\n"
            "  * Reverses a byte string. This function returns a new `string`\n"
            "    without modifying `text`.\n"
            "\n"
            "  * Returns the reversed `string`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.reverse"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_reverse(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.trim()`
    //===================================================================
    result.insert_or_assign(rocket::sref("trim"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.trim"), args);
            // Parse arguments.
            G_string text;
            Opt<G_string> reject;
            if(reader.start().g(text).g(reject).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_trim(text, reject) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.ltrim()`
    //===================================================================
    result.insert_or_assign(rocket::sref("ltrim"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.ltrim"), args);
            // Parse arguments.
            G_string text;
            Opt<G_string> reject;
            if(reader.start().g(text).g(reject).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_ltrim(text, reject) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.rtrim()`
    //===================================================================
    result.insert_or_assign(rocket::sref("rtrim"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.rtrim"), args);
            // Parse arguments.
            G_string text;
            Opt<G_string> reject;
            if(reader.start().g(text).g(reject).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_rtrim(text, reject) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.lpad()`
    //===================================================================
    result.insert_or_assign(rocket::sref("lpad"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.lpad"), args);
            // Parse arguments.
            G_string text;
            G_integer length;
            Opt<G_string> padding;
            if(reader.start().g(text).g(length).g(padding).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_lpad(text, length, padding) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.rpad()`
    //===================================================================
    result.insert_or_assign(rocket::sref("rpad"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.rpad"), args);
            // Parse arguments.
            G_string text;
            G_integer length;
            Opt<G_string> padding;
            if(reader.start().g(text).g(length).g(padding).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_rpad(text, length, padding) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.to_upper()`
    //===================================================================
    result.insert_or_assign(rocket::sref("to_upper"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.string.to_upper(text)`\n"
            "\n"
            "  * Converts all lowercase English letters in `text` to their\n"
            "    uppercase counterparts. This function returns a new `string`\n"
            "    without modifying `text`.\n"
            "\n"
            "  * Returns a new `string` after the conversion.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.to_upper"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_to_upper(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.to_lower()`
    //===================================================================
    result.insert_or_assign(rocket::sref("to_lower"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.string.to_lower(text)`\n"
            "\n"
            "  * Converts all lowercase English letters in `text` to their\n"
            "    uppercase counterparts. This function returns a new `string`\n"
            "    without modifying `text`.\n"
            "\n"
            "  * Returns a new `string` after the conversion.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.to_lower"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_to_lower(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.translate()`
    //===================================================================
    result.insert_or_assign(rocket::sref("translate"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.translate"), args);
            // Parse arguments.
            G_string text;
            G_string inputs;
            Opt<G_string> outputs;
            if(reader.start().g(text).g(inputs).g(outputs).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_translate(text, inputs, outputs) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.explode()`
    //===================================================================
    result.insert_or_assign(rocket::sref("explode"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
            "  * Throws an exception if `limit` is zero or negative.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.explode"), args);
            // Parse arguments.
            G_string text;
            Opt<G_string> delim;
            Opt<G_integer> limit;
            if(reader.start().g(text).g(delim).g(limit).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_explode(text, delim, limit) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.implode()`
    //===================================================================
    result.insert_or_assign(rocket::sref("implode"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.implode"), args);
            // Parse arguments.
            G_array segments;
            Opt<G_string> delim;
            if(reader.start().g(segments).g(delim).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_implode(segments, delim) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.hex_encode()`
    //===================================================================
    result.insert_or_assign(rocket::sref("hex_encode"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.hex_encode"), args);
            // Parse arguments.
            G_string data;
            Opt<G_boolean> lowercase;
            Opt<G_string> delim;
            if(reader.start().g(data).g(lowercase).g(delim).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_hex_encode(data, lowercase, delim) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.hex_decode()`
    //===================================================================
    result.insert_or_assign(rocket::sref("hex_decode"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.string.hex_decode(text)`\n"
            "\n"
            "  * Decodes all hexadecimal digits from `text` and converts them to\n"
            "    bytes. Whitespaces can be used to delimit bytes. Characters\n"
            "    that are neither hexadecimal digits nor whitespaces will cause\n"
            "    parse errors.\n"
            "\n"
            "  * Returns a `string` containing decoded bytes. If `text` is empty\n"
            "    or consists of only whitespaces, an empty `string` is returned.\n"
            "    In the case of parse errors, `null` is returned.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.hex_decode"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto qdata = std_string_hex_decode(text);
              if(!qdata) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qdata) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.utf8_encode()`
    //===================================================================
    result.insert_or_assign(rocket::sref("utf8_encode"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.utf8_encode"), args);
            // Parse arguments.
            G_integer code_point;
            Opt<G_boolean> permissive;
            if(reader.start().g(code_point).g(permissive).finish()) {
              // Call the binding function.
              auto qtext = std_string_utf8_encode(code_point, permissive);
              if(!qtext) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qtext) };
              return rocket::move(xref);
            }
            G_array code_points;
            if(reader.start().g(code_points).g(permissive).finish()) {
              // Call the binding function.
              auto qtext = std_string_utf8_encode(code_points, permissive);
              if(!qtext) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qtext) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.utf8_decode()`
    //===================================================================
    result.insert_or_assign(rocket::sref("utf8_decode"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.utf8_decode"), args);
            // Parse arguments.
            G_string text;
            Opt<G_boolean> permissive;
            if(reader.start().g(text).g(permissive).finish()) {
              // Call the binding function.
              auto qres = std_string_utf8_decode(text, permissive);
              if(!qres) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qres) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.pack8()`
    //===================================================================
    result.insert_or_assign(rocket::sref("pack8"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.string.pack8(values)`\n"
            "\n"
            "  * Packs a series of 8-bit integers into a `string`. `values` can\n"
            "    be either an `integer` or an `array` of `integer`s, all of\n"
            "    which are truncated to 8 bits then copied into a `string`.\n"
            "\n"
            "  * Returns the packed `string`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack8"), args);
            // Parse arguments.
            G_integer value;
            if(reader.start().g(value).finish()) {
              // Call the binding function.
              auto text = std_string_pack8(value);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            G_array values;
            if(reader.start().g(values).finish()) {
              // Call the binding function.
              auto text = std_string_pack8(values);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.unpack8()`
    //===================================================================
    result.insert_or_assign(rocket::sref("unpack8"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.string.unpack8(text)`\n"
            "\n"
            "  * Unpacks 8-bit integers from a `string`. The contents of `text`\n"
            "    are re-interpreted as contiguous signed 8-bit integers, all of\n"
            "    which are sign-extended to 64 bits then copied into an `array`.\n"
            "\n"
            "  * Returns an `array` containing unpacked integers.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack8"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto values = std_string_unpack8(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(values) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.pack_16be()`
    //===================================================================
    result.insert_or_assign(rocket::sref("pack_16be"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_16be"), args);
            // Parse arguments.
            G_integer value;
            if(reader.start().g(value).finish()) {
              // Call the binding function.
              auto text = std_string_pack_16be(value);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            G_array values;
            if(reader.start().g(values).finish()) {
              // Call the binding function.
              auto text = std_string_pack_16be(values);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.unpack_16be()`
    //===================================================================
    result.insert_or_assign(rocket::sref("unpack_16be"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_16be"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto values = std_string_unpack_16be(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(values) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.pack_16le()`
    //===================================================================
    result.insert_or_assign(rocket::sref("pack_16le"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_16le"), args);
            // Parse arguments.
            G_integer value;
            if(reader.start().g(value).finish()) {
              // Call the binding function.
              auto text = std_string_pack_16le(value);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            G_array values;
            if(reader.start().g(values).finish()) {
              // Call the binding function.
              auto text = std_string_pack_16le(values);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.unpack_16le()`
    //===================================================================
    result.insert_or_assign(rocket::sref("unpack_16le"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_16le"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto values = std_string_unpack_16le(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(values) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.pack_32be()`
    //===================================================================
    result.insert_or_assign(rocket::sref("pack_32be"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_32be"), args);
            // Parse arguments.
            G_integer value;
            if(reader.start().g(value).finish()) {
              // Call the binding function.
              auto text = std_string_pack_32be(value);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            G_array values;
            if(reader.start().g(values).finish()) {
              // Call the binding function.
              auto text = std_string_pack_32be(values);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.unpack_32be()`
    //===================================================================
    result.insert_or_assign(rocket::sref("unpack_32be"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_32be"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto values = std_string_unpack_32be(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(values) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.pack_32le()`
    //===================================================================
    result.insert_or_assign(rocket::sref("pack_32le"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_32le"), args);
            // Parse arguments.
            G_integer value;
            if(reader.start().g(value).finish()) {
              // Call the binding function.
              auto text = std_string_pack_32le(value);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            G_array values;
            if(reader.start().g(values).finish()) {
              // Call the binding function.
              auto text = std_string_pack_32le(values);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.unpack_32le()`
    //===================================================================
    result.insert_or_assign(rocket::sref("unpack_32le"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_32le"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto values = std_string_unpack_32le(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(values) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.pack_64be()`
    //===================================================================
    result.insert_or_assign(rocket::sref("pack_64be"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
            "\n"
            "`std.string.pack_64be(values)`\n"
            "\n"
            "  * Packs a series of 64-bit integers into a `string`. `values` can\n"
            "    be either an `integer` or an `array` of `integers`, all of\n"
            "    which are copied into a `string` in the big-endian byte order.\n"
            "\n"
            "  * Returns the packed `string`.\n"
          ),
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_64be"), args);
            // Parse arguments.
            G_integer value;
            if(reader.start().g(value).finish()) {
              // Call the binding function.
              auto text = std_string_pack_64be(value);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            G_array values;
            if(reader.start().g(values).finish()) {
              // Call the binding function.
              auto text = std_string_pack_64be(values);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.unpack_64be()`
    //===================================================================
    result.insert_or_assign(rocket::sref("unpack_64be"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_64be"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto values = std_string_unpack_64be(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(values) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.pack_64le()`
    //===================================================================
    result.insert_or_assign(rocket::sref("pack_64le"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_64le"), args);
            // Parse arguments.
            G_integer value;
            if(reader.start().g(value).finish()) {
              // Call the binding function.
              auto text = std_string_pack_64le(value);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            G_array values;
            if(reader.start().g(values).finish()) {
              // Call the binding function.
              auto text = std_string_pack_64le(values);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // `std.string.unpack_64le()`
    //===================================================================
    result.insert_or_assign(rocket::sref("unpack_64le"),
      G_function(make_simple_binding(
        // Description
        rocket::sref
          (
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
        // Opaque parameter
        G_null
          (
            nullptr
          ),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Reference&& /*self*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_64le"), args);
            // Parse arguments.
            G_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto values = std_string_unpack_64le(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(values) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          }
      )));
    //===================================================================
    // End of `std.string`
    //===================================================================
  }

}  // namespace Asteria
