// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_string.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"
#include <bitset>

namespace Asteria {

D_integer std_string_compare(const D_string& text1, const D_string& text2, const Opt<D_integer>& length)
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

D_boolean std_string_starts_with(const D_string& text, const D_string& prefix)
  {
    return text.starts_with(prefix);
  }

D_boolean std_string_ends_with(const D_string& text, const D_string& suffix)
  {
    return text.ends_with(suffix);
  }

D_string std_string_reverse(const D_string& text)
  {
    // This is an easy matter, isn't it?
    return D_string(text.rbegin(), text.rend());
  }

    namespace {

    std::pair<D_string::const_iterator, D_string::const_iterator> do_subrange(const D_string& text, const D_string::const_iterator& tbegin, const Opt<D_integer>& length)
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

    std::pair<D_string::const_iterator, D_string::const_iterator> do_subrange(const D_string& text, const D_integer& from, const Opt<D_integer>& length)
      {
        auto slen = static_cast<std::int64_t>(text.size());
        if(from >= 0) {
          // Behave like `std::string::substr()` except that no exception is thrown when `from` is greater than `text.size()`.
          if(from >= slen) {
            return std::make_pair(text.end(), text.end());
          }
          return do_subrange(text, text.begin() + static_cast<std::ptrdiff_t>(from), length);
        }
        // Wrap `from` from the end. Notice that `from + slen` will not overflow when `from` is negative and `slen` is not.
        auto rfrom = from + slen;
        if(rfrom >= 0) {
          // Get a subrange from the wrapped index.
          return do_subrange(text, text.begin() + static_cast<std::ptrdiff_t>(rfrom), length);
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
        return do_subrange(text, text.begin(), rfrom + *length);
      }

    }

D_string std_string_substr(const D_string& text, const D_integer& from, const Opt<D_integer>& length)
  {
    D_string res;
    auto range = do_subrange(text, from, length);
    res.assign(range.first, range.second);
    return res;
  }

D_string std_string_replace_substr(const D_string& text, const D_integer& from, const D_string& replacement)
  {
    D_string res;
    res.assign(text);
    auto range = do_subrange(res, from, rocket::nullopt);
    res.replace(range.first, range.second, replacement);
    return res;
  }

D_string std_string_replace_substr(const D_string& text, const D_integer& from, const Opt<D_integer>& length, const D_string& replacement)
  {
    D_string res;
    res.assign(text);
    auto range = do_subrange(res, from, length);
    res.replace(range.first, range.second, replacement);
    return res;
  }

    namespace {

    template<typename IteratorT> Opt<IteratorT> do_find_of(IteratorT begin, IteratorT end, const D_string& set, bool match)
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

Opt<D_integer> std_string_find_any_of(const D_string& text, const D_string& accept)
  {
    auto qit = do_find_of(text.begin(), text.end(), accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_find_any_of(const D_string& text, const Opt<D_integer>& from, const D_string& accept)
  {
    auto range = do_subrange(text, from.value_or(0), rocket::nullopt);
    auto qit = do_find_of(range.first, range.second, accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_find_any_of(const D_string& text, const Opt<D_integer>& from, const Opt<D_integer>& length, const D_string& accept)
  {
    auto range = do_subrange(text, from.value_or(0), length);
    auto qit = do_find_of(range.first, range.second, accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_rfind_any_of(const D_string& text, const D_string& accept)
  {
    auto qit = do_find_of(text.rbegin(), text.rend(), accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_rfind_any_of(const D_string& text, const Opt<D_integer>& from, const D_string& accept)
  {
    auto range = do_subrange(text, from.value_or(0), rocket::nullopt);
    auto qit = do_find_of(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_rfind_any_of(const D_string& text, const Opt<D_integer>& from, const Opt<D_integer>& length, const D_string& accept)
  {
    auto range = do_subrange(text, from.value_or(0), length);
    auto qit = do_find_of(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), accept, true);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_find_not_of(const D_string& text, const D_string& reject)
  {
    auto qit = do_find_of(text.begin(), text.end(), reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_find_not_of(const D_string& text, const Opt<D_integer>& from, const D_string& reject)
  {
    auto range = do_subrange(text, from.value_or(0), rocket::nullopt);
    auto qit = do_find_of(range.first, range.second, reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_find_not_of(const D_string& text, const Opt<D_integer>& from, const Opt<D_integer>& length, const D_string& reject)
  {
    auto range = do_subrange(text, from.value_or(0), length);
    auto qit = do_find_of(range.first, range.second, reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_rfind_not_of(const D_string& text, const D_string& reject)
  {
    auto qit = do_find_of(text.rbegin(), text.rend(), reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_rfind_not_of(const D_string& text, const Opt<D_integer>& from, const D_string& reject)
  {
    auto range = do_subrange(text, from.value_or(0), rocket::nullopt);
    auto qit = do_find_of(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

Opt<D_integer> std_string_rfind_not_of(const D_string& text, const Opt<D_integer>& from, const Opt<D_integer>& length, const D_string& reject)
  {
    auto range = do_subrange(text, from.value_or(0), length);
    auto qit = do_find_of(std::make_reverse_iterator(range.second), std::make_reverse_iterator(range.first), reject, false);
    if(!qit) {
      return rocket::nullopt;
    }
    return &**qit - text.data();
  }

    namespace {

    std::pair<const char*, std::size_t> do_what_to_trim(const Opt<D_string>& reject)
      {
        if(!reject) {
          // Delete whitespaces by default.
          return std::make_pair(" \t", 2);
        }
        // Delete characters in `*reject`.
        return std::make_pair(reject->data(), reject->size());
      }

    }

D_string std_string_trim(const D_string& text, const Opt<D_string>& reject)
  {
    auto rchars = do_what_to_trim(reject);
    if(rchars.second == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Get the index of the first byte to keep.
    auto start = text.find_first_not_of(rchars.first, rchars.second);
    if(start == D_string::npos) {
      // There is no byte to keep. Return an empty string.
      return rocket::clear;
    }
    // Get the index of the last byte to keep.
    auto end = text.find_last_not_of(rchars.first, rchars.second);
    if((start == 0) && (end == text.size() - 1)) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Return the remaining part of `text`.
    return text.substr(start, end + 1 - start);
  }

D_string std_string_ltrim(const D_string& text, const Opt<D_string>& reject)
  {
    auto rchars = do_what_to_trim(reject);
    if(rchars.second == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Get the index of the first byte to keep.
    auto start = text.find_first_not_of(rchars.first, rchars.second);
    if(start == D_string::npos) {
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

D_string std_string_rtrim(const D_string& text, const Opt<D_string>& reject)
  {
    auto rchars = do_what_to_trim(reject);
    if(rchars.second == 0) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Get the index of the last byte to keep.
    auto end = text.find_last_not_of(rchars.first, rchars.second);
    if(end == text.size() - 1) {
      // There is no byte to strip. Make use of reference counting.
      return text;
    }
    // Return the remaining part of `text`.
    return text.substr(0, end + 1);
  }

D_string std_string_to_upper(const D_string& text)
  {
    // Use reference counting as our advantage.
    D_string res = text;
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

D_string std_string_to_lower(const D_string& text)
  {
    // Use reference counting as our advantage.
    D_string res = text;
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

D_string std_string_translate(const D_string& text, const D_string& inputs, const Opt<D_string>& outputs)
  {
    // Use reference counting as our advantage.
    D_string res = text;
    char* wptr = nullptr;
    // Translate each character.
    for(std::size_t i = 0; i < res.size(); ++i) {
      char ch = res[i];
      auto ipos = inputs.find(ch);
      if(ipos == D_string::npos) {
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

D_array std_string_explode(const D_string& text, const Opt<D_string>& delim, const Opt<D_integer>& limit)
  {
    if(limit && (*limit <= 0)) {
      ASTERIA_THROW_RUNTIME_ERROR("The limit of number of segments must be greater than zero (got `", *limit, "`).");
    }
    D_array segments;
    if(text.empty()) {
      // Return an empty array.
      return segments;
    }
    if(!delim || delim->empty()) {
      // Split every byte.
      segments.reserve(text.size());
      rocket::for_each(text, [&](char ch) { segments.emplace_back(D_string(1, ch));  });
      return segments;
    }
    // Break `text` down.
    std::size_t bpos = 0;
    std::size_t epos;
    for(;;) {
      if(limit && (segments.size() >= static_cast<std::uint64_t>(*limit - 1))) {
        segments.emplace_back(text.substr(bpos));
        break;
      }
      epos = text.find(*delim, bpos);
      if(epos == Cow_String::npos) {
        segments.emplace_back(text.substr(bpos));
        break;
      }
      segments.emplace_back(text.substr(bpos, epos - bpos));
      bpos = epos + delim->size();
    }
    return segments;
  }

D_string std_string_implode(const D_array& segments, const Opt<D_string>& delim)
  {
    D_string text;
    // Deal with nasty separators.
    auto rpos = segments.begin();
    if(rpos != segments.end()) {
      for(;;) {
        text += rpos->check<D_string>();
        if(++rpos == segments.end()) {
          break;
        }
        if(delim) {
          text += *delim;
        }
      }
    }
    return text;
  }

D_string std_string_hex_encode(const D_string& text, const Opt<D_string>& delim, const Opt<D_boolean>& uppercase)
  {
    D_string hstr;
    auto rpos = text.begin();
    if(rpos == text.end()) {
      // Return an empty string; no delimiter is added.
      return hstr;
    }
    std::size_t ndcs = delim ? delim->size() : 0;
    bool upc = uppercase.value_or(false);
    // Reserve storage for digits.
    hstr.reserve(2 + (ndcs + 2) * (text.size() - 1));
    for(;;) {
      // Encode a byte.
      static constexpr char s_digits[] = "00112233445566778899aAbBcCdDeEfF";
      hstr += s_digits[(*rpos & 0xF0) / 8 + upc];
      hstr += s_digits[(*rpos & 0x0F) * 2 + upc];
      if(++rpos == text.end()) {
        break;
      }
      if(delim) {
        hstr += *delim;
      }
    }
    return hstr;
  }

Opt<D_string> std_string_hex_decode(const D_string& hstr)
  {
    D_string text;
    // Remember the value of a previous digit. `-1` means no such digit exists.
    int dprev = -1;
    for(char ch : hstr) {
      // Identify this character.
      static constexpr char s_table[] = "00112233445566778899aAbBcCdDeEfF \f\n\r\t\v";
      auto pos = std::char_traits<char>::find(s_table, sizeof(s_table) - 1, ch);
      if(!pos) {
        // Fail due to an invalid character.
        return rocket::nullopt;
      }
      auto dcur = static_cast<int>(pos - s_table) / 2;
      if(dcur >= 16) {
        // Ignore space characters.
        // But if we have had a digit, flush it.
        if(dprev != -1) {
          text.push_back(static_cast<char>(dprev));
          dprev = -1;
        }
        continue;
      }
      if(dprev == -1) {
        // Save this digit.
        dprev = dcur;
        continue;
      }
      // We have got two digits now.
      // Make a byte and write it.
      text.push_back(static_cast<char>(dprev * 16 + dcur));
      dprev = -1;
    }
    // If we have had a digit, flush it.
    if(dprev != -1) {
      text.push_back(static_cast<char>(dprev));
      dprev = -1;
    }
    return rocket::move(text);
  }

Opt<D_string> std_string_utf8_encode(const D_array& code_points, const Opt<D_boolean>& permissive)
  {
    D_string text;
    for(std::size_t i = 0; i < code_points.size(); ++i) {
      // Encode each code point.
      auto value = code_points[i].check<D_integer>();
      if(((0xD800 <= value) && (value < 0xE000)) || (0x110000 <= value)) {
        // Code point value is reserved or too large.
        if(permissive != true) {
          return rocket::nullopt;
        }
        // Replace it with the replacement character.
        value = 0xFFFD;
      }
      char32_t cpnt = value & 0x1FFFFF;
      // Encode it.
      auto encode_one = [&](unsigned shift, unsigned mask)
        {
          text.push_back(static_cast<char>((~mask << 1) | ((cpnt >> shift) & mask)));
        };
      if(cpnt < 0x80) {
        encode_one( 0, 0xFF);
        continue;
      }
      if(cpnt < 0x800) {
        encode_one( 6, 0x1F);
        encode_one( 0, 0x3F);
        continue;
      }
      if(cpnt < 0x10000) {
        encode_one(12, 0x0F);
        encode_one( 6, 0x3F);
        encode_one( 0, 0x3F);
        continue;
      }
      encode_one(18, 0x07);
      encode_one(12, 0x3F);
      encode_one( 6, 0x3F);
      encode_one( 0, 0x3F);
    }
    return rocket::move(text);
  }

Opt<D_array> std_string_utf8_decode(const D_string& text, const Opt<D_boolean>& permissive)
  {
    D_array code_points;
    for(std::size_t i = 0; i < text.size(); ++i) {
      // Read the first byte.
      char32_t cpnt = text[i] & 0xFF;
      if(cpnt < 0x80) {
        // This sequence contains only one byte.
        code_points.emplace_back(D_integer(cpnt));
        continue;
      }
      if((cpnt < 0xC0) || (0xF8 <= cpnt)) {
        // This is not a leading character.
        if(permissive != true) {
          return rocket::nullopt;
        }
        // Re-interpret it as an isolated byte.
        code_points.emplace_back(D_integer(cpnt));
        continue;
      }
      // Calculate the number of bytes in this code point.
      auto u8len = static_cast<std::size_t>(2 + (cpnt >= 0xE0) + (cpnt >= 0xF0));
      ROCKET_ASSERT(u8len >= 2);
      ROCKET_ASSERT(u8len <= 4);
      if(u8len > text.size() - i) {
        // No enough characters have been provided.
        if(permissive != true) {
          return rocket::nullopt;
        }
        // Re-interpret it as an isolated byte.
        code_points.emplace_back(D_integer(cpnt));
        continue;
      }
      // Unset bits that are not part of the payload.
      cpnt &= UINT32_C(0xFF) >> u8len;
      // Accumulate trailing code units.
      std::size_t k;
      for(k = 1; k < u8len; ++k) {
        char32_t next = text[++i] & 0xFF;
        if((next < 0x80) || (0xC0 <= next)) {
          // This trailing character is not valid.
          break;
        }
        cpnt = (cpnt << 6) | (next & 0x3F);
      }
      if(k != u8len) {
        // An error has been encountered when parsing trailing characters.
        if(permissive != true) {
          return rocket::nullopt;
        }
        // Replace this character.
        code_points.emplace_back(D_integer(0xFFFD));
        continue;
      }
      if(((0xD800 <= cpnt) && (cpnt < 0xE000)) || (0x110000 <= cpnt)) {
        // Code point value is reserved or too large.
        if(permissive != true) {
          return rocket::nullopt;
        }
        // Replace this character.
        code_points.emplace_back(D_integer(0xFFFD));
        continue;
      }
      // Re-encode it and check for overlong sequences.
      auto mlen = static_cast<std::size_t>(1 + (cpnt >= 0x80) + (cpnt >= 0x800) + (cpnt >= 0x10000));
      if(mlen != u8len) {
        // Overlong sequences are not allowed.
        if(permissive != true) {
          return rocket::nullopt;
        }
        // Replace this character.
        code_points.emplace_back(D_integer(0xFFFD));
        continue;
      }
      code_points.emplace_back(D_integer(cpnt));
    }
    return rocket::move(code_points);
  }

    namespace {

    template<typename IntegerT, bool bigendT> D_string do_pack(const D_array& ints)
      {
        D_string text;
        // Define temporary storage.
        std::array<char, sizeof(IntegerT)> stor_le;
        std::uint64_t word = 0;
        // How many words will the result have?
        auto nwords = ints.size();
        if(nwords > text.max_size() / stor_le.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("The result string could not be allocated (requesting `", nwords, "` instances of size `", stor_le.size(), "`).");
        }
        text.reserve(stor_le.size() * nwords);
        // Pack integers.
        for(std::size_t i = 0; i < nwords; ++i) {
          // Read an integer.
          word = static_cast<std::uint64_t>(ints.at(i).check<D_integer>());
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
        }
        return text;
      }

    template<typename IntegerT, bool bigendT> D_array do_unpack(const D_string& text)
      {
        D_array ints;
        // Define temporary storage.
        std::array<char, sizeof(IntegerT)> stor_be;
        std::uint64_t word = 0;
        // How many words will the result have?
        auto nwords = text.size() / stor_be.size();
        if(text.size() != nwords * stor_be.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("The length of the source string must be a multiple of `", stor_be.size(), "` (got `", text.size(), "`).");
        }
        ints.reserve(nwords);
        // Unpack integers.
        for(std::size_t i = 0; i < nwords; ++i) {
          // Read some bytes in big-endian order.
          if(bigendT) {
            std::copy_n(text.data() + i * stor_be.size(), stor_be.size(), stor_be.begin());
          } else {
            std::copy_n(text.data() + i * stor_be.size(), stor_be.size(), stor_be.rbegin());
          }
          // Assembly the word.
          for(const auto& byte : stor_be) {
            word <<= 8;
            word |= static_cast<unsigned char>(byte);
          }
          // Append the word.
          ints.emplace_back(D_integer(static_cast<IntegerT>(word)));
        }
        return ints;
      }

    }

D_string std_string_pack_8(const D_array& ints)
  {
    return do_pack<std::int8_t, false>(ints);
  }

D_array std_string_unpack_8(const D_string& text)
  {
    return do_unpack<std::int8_t, false>(text);
  }

D_string std_string_pack_16be(const D_array& ints)
  {
    return do_pack<std::int16_t, true>(ints);
  }

D_array std_string_unpack_16be(const D_string& text)
  {
    return do_unpack<std::int16_t, true>(text);
  }

D_string std_string_pack_16le(const D_array& ints)
  {
    return do_pack<std::int16_t, false>(ints);
  }

D_array std_string_unpack_16le(const D_string& text)
  {
    return do_unpack<std::int16_t, false>(text);
  }

D_string std_string_pack_32be(const D_array& ints)
  {
    return do_pack<std::int32_t, true>(ints);
  }

D_array std_string_unpack_32be(const D_string& text)
  {
    return do_unpack<std::int32_t, true>(text);
  }

D_string std_string_pack_32le(const D_array& ints)
  {
    return do_pack<std::int32_t, false>(ints);
  }

D_array std_string_unpack_32le(const D_string& text)
  {
    return do_unpack<std::int32_t, false>(text);
  }

D_string std_string_pack_64be(const D_array& ints)
  {
    return do_pack<std::int64_t, true>(ints);
  }

D_array std_string_unpack_64be(const D_string& text)
  {
    return do_unpack<std::int64_t, true>(text);
  }

D_string std_string_pack_64le(const D_array& ints)
  {
    return do_pack<std::int64_t, false>(ints);
  }

D_array std_string_unpack_64le(const D_string& text)
  {
    return do_unpack<std::int64_t, false>(text);
  }

D_object create_bindings_string()
  {
    D_object ro;
    //===================================================================
    // `std.string.compare()`
    //===================================================================
    ro.try_emplace(rocket::sref("compare"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.compare(text1, text2, [length])`\n"
                     "  * Performs lexicographical comparison on two byte `string`s. If\n"
                     "    `length` is set to an `integer`, no more than this number of\n"
                     "    bytes are compared. This function behaves like the `strncmp()`\n"
                     "    function in C, except that null characters do not terminate\n"
                     "    strings.\n"
                     "  * Returns a positive `integer` if `text1` compares greater than\n"
                     "    `text2`, a negative `integer` if `text1` compares less than\n"
                     "    `text2`, or zero if `text1` compares equal to `text2`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.compare"), args);
            // Parse arguments.
            D_string text1;
            D_string text2;
            Opt<D_integer> length;
            if(reader.start().g(text1).g(text2).g(length).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_compare(text1, text2, length) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.starts_with()`
    //===================================================================
    ro.try_emplace(rocket::sref("starts_with"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.starts_with(text, prefix)`\n"
                     "  * Checks whether `prefix` is a prefix of `text`. The empty\n"
                     "    `string` is considered to be a prefix of any string.\n"
                     "  * Returns `true` if `prefix` is a prefix of `text`; otherwise\n"
                     "    `false`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.starts_with"), args);
            // Parse arguments.
            D_string text;
            D_string prefix;
            if(reader.start().g(text).g(prefix).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_starts_with(text, prefix) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.ends_with()`
    //===================================================================
    ro.try_emplace(rocket::sref("ends_with"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.ends_with(text, suffix)`\n"
                     "  * Checks whether `suffix` is a suffix of `text`. The empty\n"
                     "    `string` is considered to be a suffix of any string.\n"
                     "  * Returns `true` if `suffix` is a suffix of `text`; otherwise\n"
                     "    `false`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.ends_with"), args);
            // Parse arguments.
            D_string text;
            D_string suffix;
            if(reader.start().g(text).g(suffix).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_ends_with(text, suffix) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.substr()`
    //===================================================================
    ro.try_emplace(rocket::sref("substr"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.substr(text, from, [length])`\n"
                     "  * Copies a subrange of `text` to create a new byte string. Bytes\n"
                     "    are copied from `from` if it is non-negative, and from\n"
                     "    `lengthof(text) + from` otherwise. If `length` is set to an\n"
                     "    `integer`, no more than this number of bytes will be copied. If\n"
                     "    it is absent, all bytes from `from` to the end of `text` will\n"
                     "    be copied. If `from` is outside `text`, an empty `string` is\n"
                     "    returned.\n"
                     "  * Returns the specified substring of `text`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.substr"), args);
            // Parse arguments.
            D_string text;
            D_integer from;
            Opt<D_integer> length;
            if(reader.start().g(text).g(from).g(length).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_substr(text, from, length) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.replace_substr()`
    //===================================================================
    ro.try_emplace(rocket::sref("replace_substr"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.replace_substr(text, from, replacement)`\n"
                     "  * Replaces all bytes from `from` to the end of `text` with\n"
                     "    `replacement` and returns the new byte string. If `from` is\n"
                     "    negative, it specifies an offset from the end of `text`. This\n"
                     "    function returns a new `string` without modifying `text`.\n"
                     "  * Returns a `string` with the subrange replaced.\n"
                     "`std.string.replace_substr(text, from, [length], replacement)`\n"
                     "  * Replaces all bytes from `from` to the end of `text` with\n"
                     "    `replacement` and returns the new byte string. If `from` is\n"
                     "    negative, it specifies an offset from the end of `text`. If\n"
                     "    `length` is set to an `integer`, no more than this number of\n"
                     "    bytes will be copied. If it is absent, this function is\n"
                     "    equivalent to `replace_substr(text, from, replacement)`. This\n"
                     "    function returns a new `string` without modifying `text`.\n"
                     "  * Returns a `string` with the subrange replaced.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.replace"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_string text;
            D_integer from;
            D_string replacement;
            if(reader.start().g(text).g(from).save_state(state).g(replacement).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_replace_substr(text, from, replacement) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(replacement).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_replace_substr(text, from, length, replacement) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.find_any_of()`
    //===================================================================
    ro.try_emplace(rocket::sref("find_any_of"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.find_any_of(text, accept)`\n"
                     "  * Searches `text` for bytes that exist in `accept`.\n"
                     "  * Returns the subscript of the first byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"
                     "`std.string.find_any_of(text, [from], accept)`\n"
                     "  * Searches `text` for bytes that exist in `accept`. The search\n"
                     "    operation is performed on the same subrange that would be\n"
                     "    returned by `substr(text, from)`.\n"
                     "  * Returns the subscript of the first byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"
                     "`std.string.find_any_of(text, [from], [length], accept)`\n"
                     "  * Searches `text` for bytes that exist in `accept`. The search\n"
                     "    operation is performed on the same subrange that would be\n"
                     "    returned by `substr(text, from, length)`.\n"
                     "  * Returns the subscript of the first byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.find_any_of"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_string text;
            D_string accept;
            if(reader.start().g(text).save_state(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_find_any_of(text, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> from;
            if(reader.load_state(state).g(from).save_state(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_find_any_of(text, from, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(accept).finish()) {
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
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.rfind_any_of()`
    //===================================================================
    ro.try_emplace(rocket::sref("rfind_any_of"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.rfind_any_of(text, accept)`\n"
                     "  * Searches `text` for bytes that exist in `accept`.\n"
                     "  * Returns the subscript of the last byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"
                     "`std.string.rfind_any_of(text, [from], accept)`\n"
                     "  * Searches `text` for bytes that exist in `accept`. The search\n"
                     "    operation is performed on the same subrange that would be\n"
                     "    returned by `substr(text, from)`.\n"
                     "  * Returns the subscript of the last byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"
                     "`std.string.rfind_any_of(text, [from], [length], accept)`\n"
                     "  * Searches `text` for bytes that exist in `accept`. The search\n"
                     "    operation is performed on the same subrange that would be\n"
                     "    returned by `substr(text, from, length)`.\n"
                     "  * Returns the subscript of the last byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.rfind_any_of"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_string text;
            D_string accept;
            if(reader.start().g(text).save_state(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind_any_of(text, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> from;
            if(reader.load_state(state).g(from).save_state(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind_any_of(text, from, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(accept).finish()) {
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
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.find_not_of()`
    //===================================================================
    ro.try_emplace(rocket::sref("find_not_of"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.find_not_of(text, reject)`\n"
                     "  * Searches `text` for bytes that does not exist in `reject`.\n"
                     "  * Returns the subscript of the first byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"
                     "`std.string.find_not_of(text, [from], reject)`\n"
                     "  * Searches `text` for bytes that does not exist in `reject`. The\n"
                     "    search operation is performed on the same subrange that would\n"
                     "    be returned by `substr(text, from)`.\n"
                     "  * Returns the subscript of the first byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"
                     "`std.string.find_not_of(text, [from], [length], reject)`\n"
                     "  * Searches `text` for bytes that does not exist in `reject`. The\n"
                     "    search operation is performed on the same subrange that would\n"
                     "    be returned by `substr(text, from, length)`.\n"
                     "  * Returns the subscript of the first byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.find_not_of"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_string text;
            D_string accept;
            if(reader.start().g(text).save_state(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_find_not_of(text, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> from;
            if(reader.load_state(state).g(from).save_state(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_find_not_of(text, from, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(accept).finish()) {
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
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.rfind_not_of()`
    //===================================================================
    ro.try_emplace(rocket::sref("rfind_not_of"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.rfind_not_of(text, reject)`\n"
                     "  * Searches `text` for bytes that does not exist in `reject`.\n"
                     "  * Returns the subscript of the last byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"
                     "`std.string.rfind_not_of(text, [from], reject)`\n"
                     "  * Searches `text` for bytes that does not exist in `reject`. The\n"
                     "    search operation is performed on the same subrange that would\n"
                     "    be returned by `substr(text, from)`.\n"
                     "  * Returns the subscript of the last byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"
                     "`std.string.rfind_not_of(text, [from], [length], reject)`\n"
                     "  * Searches `text` for bytes that does not exist in `reject`. The\n"
                     "    search operation is performed on the same subrange that would\n"
                     "    be returned by `substr(text, from, length)`.\n"
                     "  * Returns the subscript of the last byte found, which is always\n"
                     "    non-negative; or `null` if no such byte exists.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.rfind_not_of"), args);
            Argument_Reader::State state;
            // Parse arguments.
            D_string text;
            D_string accept;
            if(reader.start().g(text).save_state(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind_not_of(text, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> from;
            if(reader.load_state(state).g(from).save_state(state).g(accept).finish()) {
              // Call the binding function.
              auto qindex = std_string_rfind_not_of(text, from, accept);
              if(!qindex) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qindex) };
              return rocket::move(xref);
            }
            Opt<D_integer> length;
            if(reader.load_state(state).g(length).g(accept).finish()) {
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
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.reverse()`
    //===================================================================
    ro.try_emplace(rocket::sref("reverse"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.reverse(text)`\n"
                     "  * Reverses a byte `string`. This function returns a new `string`\n"
                     "    without modifying `text`.\n"
                     "  * Returns the reversed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.reverse"), args);
            // Parse arguments.
            D_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_reverse(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.trim()`
    //===================================================================
    ro.try_emplace(rocket::sref("trim"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.trim(text, [reject])`\n"
                     "  * Removes the longest prefix and suffix consisting solely bytes\n"
                     "    from `reject`. If `reject` is empty, no byte is removed. If\n"
                     "    `reject` is not specified, spaces and tabs are removed. This\n"
                     "    function returns a new `string` without modifying `text`.\n"
                     "  * Returns the trimmed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.trim"), args);
            // Parse arguments.
            D_string text;
            Opt<D_string> reject;
            if(reader.start().g(text).g(reject).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_trim(text, reject) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.ltrim()`
    //===================================================================
    ro.try_emplace(rocket::sref("ltrim"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.ltrim(text, [reject])`\n"
                     "  * Removes the longest prefix consisting solely bytes from\n"
                     "    `reject`. If `reject` is empty, no byte is removed. If `reject`\n"
                     "    is not specified, spaces and tabs are removed. This function\n"
                     "    returns a new `string` without modifying `text`.\n"
                     "  * Returns the trimmed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.ltrim"), args);
            // Parse arguments.
            D_string text;
            Opt<D_string> reject;
            if(reader.start().g(text).g(reject).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_ltrim(text, reject) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.rtrim()`
    //===================================================================
    ro.try_emplace(rocket::sref("rtrim"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.rtrim(text, [reject])`\n"
                     "  * Removes the longest suffix consisting solely bytes from\n"
                     "    `reject`. If `reject` is empty, no byte is removed. If `reject`\n"
                     "    is not specified, spaces and tabs are removed. This function\n"
                     "    returns a new `string` without modifying `text`.\n"
                     "  * Returns the trimmed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.rtrim"), args);
            // Parse arguments.
            D_string text;
            Opt<D_string> reject;
            if(reader.start().g(text).g(reject).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_rtrim(text, reject) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.to_upper()`
    //===================================================================
    ro.try_emplace(rocket::sref("to_upper"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.to_upper(text)`\n"
                     "  * Converts all lowercase English letters in `text` to their\n"
                     "    uppercase counterparts. This function returns a new `string`\n"
                     "    without modifying `text`.\n"
                     "  * Returns a new `string` after the conversion.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.to_upper"), args);
            // Parse arguments.
            D_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_to_upper(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.to_lower()`
    //===================================================================
    ro.try_emplace(rocket::sref("to_lower"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.to_lower(text)`\n"
                     "  * Converts all lowercase English letters in `text` to their\n"
                     "    uppercase counterparts. This function returns a new `string`\n"
                     "    without modifying `text`.\n"
                     "  * Returns a new `string` after the conversion.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.to_lower"), args);
            // Parse arguments.
            D_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_to_lower(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.translate()`
    //===================================================================
    ro.try_emplace(rocket::sref("translate"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("``std.string.translate(text, inputs, [outputs])`\n"
                     "  * Performs bytewise translation on the given string. For every\n"
                     "    byte in `text` that is also found in `inputs`, if there is a\n"
                     "    corresponding replacement byte in `outputs` with the same\n"
                     "    subscript, it is replaced with the latter; if no replacement\n"
                     "    exists, because `outputs` is shorter than `inputs` or is null,\n"
                     "    it is deleted. If `outputs` is longer than `inputs`, excess\n"
                     "    bytes are ignored. Bytes that do not exist in `inputs` are left\n"
                     "    intact. This function returns a new `string` without modifying\n"
                     "    `text`.\n"
                     "  * Returns the translated `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.translate"), args);
            // Parse arguments.
            D_string text;
            D_string inputs;
            Opt<D_string> outputs;
            if(reader.start().g(text).g(inputs).g(outputs).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_translate(text, inputs, outputs) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.explode()`
    //===================================================================
    ro.try_emplace(rocket::sref("explode"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.explode(text, [delim], [limit])`\n"
                     "  * Breaks `text` down into segments, separated by `delim`. If\n"
                     "    `delim` is `null` or an empty `string`, every byte becomes a\n"
                     "    segment. If `limit` is set to a positive `integer`, there will\n"
                     "    be no more segments than this number; the last segment will\n"
                     "    contain all the remaining bytes of the `text`.\n"
                     "  * Returns an `array` containing the broken-down segments. If\n"
                     "    `text` is empty, an empty `array` is returned.\n"
                     "  * Throws an exception if `limit` is zero or negative.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.explode"), args);
            // Parse arguments.
            D_string text;
            Opt<D_string> delim;
            Opt<D_integer> limit;
            if(reader.start().g(text).g(delim).g(limit).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_explode(text, delim, limit) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.implode()`
    //===================================================================
    ro.try_emplace(rocket::sref("implode"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.implode(segments, [delim])`\n"
                     "  * Concatenates elements of an array, `segments`, to create a new\n"
                     "    `string`. All segments shall be `string`s. If `delim` is\n"
                     "    specified, it is inserted between adjacent segments.\n"
                     "  * Returns a `string` containing all segments. If `segments` is\n"
                     "    empty, an empty `string` is returned.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.implode"), args);
            // Parse arguments.
            D_array segments;
            Opt<D_string> delim;
            if(reader.start().g(segments).g(delim).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_implode(segments, delim) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.hex_encode()`
    //===================================================================
    ro.try_emplace(rocket::sref("hex_encode"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.hex_encode(text, [delim], [uppercase])`\n"
                     "  * Encodes all bytes in `text` as 2-digit hexadecimal numbers and\n"
                     "    concatenates them. If `delim` is specified, it is inserted\n"
                     "    between adjacent bytes. If `uppercase` is set to `true`,\n"
                     "    hexadecimal digits above `9` are encoded as `ABCDEF`; otherwise\n"
                     "    they are encoded as `abcdef`.\n"
                     "  * Returns the encoded `string`. If `text` is empty, an empty\n"
                     "    `string` is returned.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.hex_encode"), args);
            // Parse arguments.
            D_string text;
            Opt<D_string> delim;
            Opt<D_boolean> uppercase;
            if(reader.start().g(text).g(delim).g(uppercase).finish()) {
              // Call the binding function.
              Reference_Root::S_temporary xref = { std_string_hex_encode(text, delim, uppercase) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.hex_decode()`
    //===================================================================
    ro.try_emplace(rocket::sref("hex_decode"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.hex_decode(hstr)`\n"
                     "  * Decodes all hexadecimal digits from `hstr` and converts them to\n"
                     "    bytes. Whitespaces are ignored. Characters that are neither\n"
                     "    hexadecimal digits nor whitespaces will cause parse errors.\n"
                     "  * Returns a `string` containing decoded bytes. If `hstr` is empty\n"
                     "    or consists only whitespaces, an empty `string` is returned. In\n"
                     "    the case of parse errors, `null` is returned.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.hex_decode"), args);
            // Parse arguments.
            D_string hstr;
            if(reader.start().g(hstr).finish()) {
              // Call the binding function.
              auto qtext = std_string_hex_decode(hstr);
              if(!qtext) {
                return Reference_Root::S_null();
              }
              Reference_Root::S_temporary xref = { rocket::move(*qtext) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.utf8_encode()`
    //===================================================================
    ro.try_emplace(rocket::sref("utf8_encode"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.utf8_encode(code_points, [permissive])`\n"
                     "  * Encodes code points from `code_points` into an UTF-8 `string`.\n"
                     "    Code points shall be `integer`s. When an invalid code point is\n"
                     "    encountered, if `permissive` is set to `true`, it is replaced\n"
                     "    with the replacement character `\"\\uFFFD\"` and consequently\n"
                     "    encoded as `\"\\xEF\\xBF\\xBD\"`; otherwise this function fails.\n"
                     "  * Returns the encoded `string` on success; otherwise `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.utf8_encode"), args);
            // Parse arguments.
            D_array code_points;
            Opt<D_boolean> permissive;
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
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.utf8_decode()`
    //===================================================================
    ro.try_emplace(rocket::sref("utf8_decode"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.utf8_decode(text, [permissive])`\n"
                     "  * Decodes `text`, which is expected to be a `string` containing\n"
                     "    UTF-8 code units, into an `array` of code points, represented\n"
                     "    as `integer`s. When an invalid code sequence is encountered, if\n"
                     "    `permissive` is set to `true`, all code units of it are\n"
                     "    re-interpreted as isolated bytes according to ISO/IEC 8859-1;\n"
                     "    otherwise this function fails.\n"
                     "  * Returns an `array` containing decoded code points; otherwise\n"
                     "    `null`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.utf8_decode"), args);
            // Parse arguments.
            D_string text;
            Opt<D_boolean> permissive;
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
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.pack_8()`
    //===================================================================
    ro.try_emplace(rocket::sref("pack_8"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.pack_8(ints)`\n"
                     "  * Packs a series of 8-bit integers into a `string`. `ints` shall\n"
                     "    be an `array` of `integer`s, all of which are truncated to 8\n"
                     "    bits then copied into a `string`.\n"
                     "  * Returns the packed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_8"), args);
            // Parse arguments.
            D_array ints;
            if(reader.start().g(ints).finish()) {
              // Call the binding function.
              auto text = std_string_pack_8(ints);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.unpack_8()`
    //===================================================================
    ro.try_emplace(rocket::sref("unpack_8"),
      D_function(make_simple_binding(
        rocket::sref("`std.string.unpack_8(text)`\n"
                     "  * Unpacks 8-bit integers from a `string`. The contents of `text`\n"
                     "    are re-interpreted as contiguous signed 8-bit integers, all of\n"
                     "    which are sign-extended to 64 bits then copied into an `array`.\n"
                     "  * Returns an `array` containing unpacked integers.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_8"), args);
            // Parse arguments.
            D_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto ints = std_string_unpack_8(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(ints) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.pack_16be()`
    //===================================================================
    ro.try_emplace(rocket::sref("pack_16be"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.pack_16be(ints)`\n"
                     "  * Packs a series of 16-bit integers into a `string`. `ints` shall\n"
                     "    be an `array` of `integer`s, all of which are truncated to 16\n"
                     "    bits then copied into a `string` in the big-endian byte order.\n"
                     "  * Returns the packed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_16be"), args);
            // Parse arguments.
            D_array ints;
            if(reader.start().g(ints).finish()) {
              // Call the binding function.
              auto text = std_string_pack_16be(ints);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.unpack_16be()`
    //===================================================================
    ro.try_emplace(rocket::sref("unpack_16be"),
      D_function(make_simple_binding(
        rocket::sref("`std.string.unpack_16be(text)`\n"
                     "  * Unpacks 16-bit integers from a `string`. The contents of `text`\n"
                     "    are re-interpreted as contiguous signed 16-bit integers in the\n"
                     "    big-endian byte order, all of which are sign-extended to 64\n"
                     "    bits then copied into an `array`.\n"
                     "  * Returns an `array` containing unpacked integers.\n"
                     "  * Throws an exception if the length of `text` is not a multiple\n"
                     "    of 2.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_16be"), args);
            // Parse arguments.
            D_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto ints = std_string_unpack_16be(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(ints) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.pack_16le()`
    //===================================================================
    ro.try_emplace(rocket::sref("pack_16le"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.pack_16le(ints)`\n"
                     "  * Packs a series of 16-bit integers into a `string`. `ints` shall\n"
                     "    be an `array` of `integer`s, all of which are truncated to 16\n"
                     "    bits then copied into a `string` in the little-endian byte\n"
                     "    order.\n"
                     "  * Returns the packed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_16le"), args);
            // Parse arguments.
            D_array ints;
            if(reader.start().g(ints).finish()) {
              // Call the binding function.
              auto text = std_string_pack_16le(ints);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.unpack_16le()`
    //===================================================================
    ro.try_emplace(rocket::sref("unpack_16le"),
      D_function(make_simple_binding(
        rocket::sref("`std.string.unpack_16le(text)`\n"
                     "  * Unpacks 16-bit integers from a `string`. The contents of `text`\n"
                     "    are re-interpreted as contiguous signed 16-bit integers in the\n"
                     "    little-endian byte order, all of which are sign-extended to 64\n"
                     "    bits then copied into an `array`.\n"
                     "  * Returns an `array` containing unpacked integers.\n"
                     "  * Throws an exception if the length of `text` is not a multiple\n"
                     "    of 2.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_16le"), args);
            // Parse arguments.
            D_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto ints = std_string_unpack_16le(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(ints) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.pack_32be()`
    //===================================================================
    ro.try_emplace(rocket::sref("pack_32be"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.pack_32be(ints)`\n"
                     "  * Packs a series of 32-bit integers into a `string`. `ints` shall\n"
                     "    be an `array` of `integer`s, all of which are truncated to 32\n"
                     "    bits then copied into a `string` in the big-endian byte order.\n"
                     "  * Returns the packed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_32be"), args);
            // Parse arguments.
            D_array ints;
            if(reader.start().g(ints).finish()) {
              // Call the binding function.
              auto text = std_string_pack_32be(ints);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.unpack_32be()`
    //===================================================================
    ro.try_emplace(rocket::sref("unpack_32be"),
      D_function(make_simple_binding(
        rocket::sref("`std.string.unpack_32be(text)`\n"
                     "  * Unpacks 32-bit integers from a `string`. The contents of `text`\n"
                     "    are re-interpreted as contiguous signed 32-bit integers in the\n"
                     "    big-endian byte order, all of which are sign-extended to 64\n"
                     "    bits then copied into an `array`.\n"
                     "  * Returns an `array` containing unpacked integers.\n"
                     "  * Throws an exception if the length of `text` is not a multiple\n"
                     "    of 4.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_32be"), args);
            // Parse arguments.
            D_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto ints = std_string_unpack_32be(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(ints) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.pack_32le()`
    //===================================================================
    ro.try_emplace(rocket::sref("pack_32le"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.pack_32le(ints)`\n"
                     "  * Packs a series of 32-bit integers into a `string`. `ints` shall\n"
                     "    be an `array` of `integer`s, all of which are truncated to 32\n"
                     "    bits then copied into a `string` in the little-endian byte\n"
                     "    order.\n"
                     "  * Returns the packed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_32le"), args);
            // Parse arguments.
            D_array ints;
            if(reader.start().g(ints).finish()) {
              // Call the binding function.
              auto text = std_string_pack_32le(ints);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.unpack_32le()`
    //===================================================================
    ro.try_emplace(rocket::sref("unpack_32le"),
      D_function(make_simple_binding(
        rocket::sref("`std.string.unpack_32le(text)`\n"
                     "  * Unpacks 32-bit integers from a `string`. The contents of `text`\n"
                     "    are re-interpreted as contiguous signed 32-bit integers in the\n"
                     "    little-endian byte order, all of which are sign-extended to 64\n"
                     "    bits then copied into an `array`.\n"
                     "  * Returns an `array` containing unpacked integers.\n"
                     "  * Throws an exception if the length of `text` is not a multiple\n"
                     "    of 4.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_32le"), args);
            // Parse arguments.
            D_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto ints = std_string_unpack_32le(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(ints) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.pack_64be()`
    //===================================================================
    ro.try_emplace(rocket::sref("pack_64be"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.pack_64be(ints)`\n"
                     "  * Packs a series of 64-bit integers into a `string`. `ints` shall\n"
                     "    be an `array` of `integer`s, all of which are copied into a\n"
                     "    `string` in the big-endian byte order.\n"
                     "  * Returns the packed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_64be"), args);
            // Parse arguments.
            D_array ints;
            if(reader.start().g(ints).finish()) {
              // Call the binding function.
              auto text = std_string_pack_64be(ints);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.unpack_64be()`
    //===================================================================
    ro.try_emplace(rocket::sref("unpack_64be"),
      D_function(make_simple_binding(
        rocket::sref("`std.string.unpack_64be(text)`\n"
                     "  * Unpacks 64-bit integers from a `string`. The contents of `text`\n"
                     "    are re-interpreted as contiguous signed 64-bit integers in the\n"
                     "    big-endian byte order, all of which are copied into an `array`.\n"
                     "  * Returns an `array` containing unpacked integers.\n"
                     "  * Throws an exception if the length of `text` is not a multiple\n"
                     "    of 8.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_64be"), args);
            // Parse arguments.
            D_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto ints = std_string_unpack_64be(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(ints) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.pack_64le()`
    //===================================================================
    ro.try_emplace(rocket::sref("pack_64le"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.string.pack_64le(ints)`\n"
                     "  * Packs a series of 64-bit integers into a `string`. `ints` shall\n"
                     "    be an `array` of `integer`s, all of which are copied into a\n"
                     "    `string` in the little-endian byte order.\n"
                     "  * Returns the packed `string`.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.pack_64le"), args);
            // Parse arguments.
            D_array ints;
            if(reader.start().g(ints).finish()) {
              // Call the binding function.
              auto text = std_string_pack_64le(ints);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(text) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // `std.string.unpack_64le()`
    //===================================================================
    ro.try_emplace(rocket::sref("unpack_64le"),
      D_function(make_simple_binding(
        rocket::sref("`std.string.unpack_64le(text)`\n"
                     "  * Unpacks 64-bit integers from a `string`. The contents of `text`\n"
                     "    are re-interpreted as contiguous signed 64-bit integers in the\n"
                     "    little-endian byte order, all of which are copied into an\n"
                     "    `array`.\n"
                     "  * Returns an `array` containing unpacked integers.\n"
                     "  * Throws an exception if the length of `text` is not a multiple\n"
                     "    of 8.\n"),
        // Definition
        [](const Value& /*opaque*/, const Global_Context& /*global*/, Cow_Vector<Reference>&& args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.string.unpack_64le"), args);
            // Parse arguments.
            D_string text;
            if(reader.start().g(text).finish()) {
              // Call the binding function.
              auto ints = std_string_unpack_64le(text);
              // Forward the result.
              Reference_Root::S_temporary xref = { rocket::move(ints) };
              return rocket::move(xref);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameter
        D_null()
      )));
    //===================================================================
    // End of `std.string`
    //===================================================================
    return ro;
  }

}  // namespace Asteria
