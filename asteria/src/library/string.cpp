// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "string.hpp"
#include "../runtime/argument_reader.hpp"
#include "../utilities.hpp"
#include <endian.h>
#include <regex>

namespace asteria {
namespace {

pair<V_string::const_iterator, V_string::const_iterator>
do_slice(const V_string& text, V_string::const_iterator tbegin, const optV_integer& length)
  {
    if(!length || (*length >= text.end() - tbegin))
      return ::std::make_pair(tbegin, text.end());

    if(*length <= 0)
      return ::std::make_pair(tbegin, tbegin);

    // Don't go past the end.
    return ::std::make_pair(tbegin, tbegin + static_cast<ptrdiff_t>(*length));
  }

pair<V_string::const_iterator, V_string::const_iterator>
do_slice(const V_string& text, const V_integer& from, const optV_integer& length)
  {
    auto slen = static_cast<int64_t>(text.size());
    if(from >= 0) {
      // Behave like `::std::string::substr()` except that no exception is thrown when `from` is
      // greater than `text.size()`.
      if(from >= slen)
        return ::std::make_pair(text.end(), text.end());

      return do_slice(text, text.begin() + static_cast<ptrdiff_t>(from), length);
    }

    // Wrap `from` from the end. Notice that `from + slen` will not overflow when `from` is negative
    // and `slen` is not.
    auto rfrom = from + slen;
    if(rfrom >= 0)
      return do_slice(text, text.begin() + static_cast<ptrdiff_t>(rfrom), length);

    // Get a subrange from the beginning of `text`, if the wrapped index is before the first byte.
    if(!length)
      return ::std::make_pair(text.begin(), text.end());

    if(*length <= 0)
      return ::std::make_pair(text.begin(), text.begin());

    // Get a subrange excluding the part before the beginning.
    // Notice that `rfrom + *length` will not overflow when `rfrom` is negative and `*length` is not.
    return do_slice(text, text.begin(), rfrom + *length);
  }

// https://en.wikipedia.org/wiki/Boyer-Moore-Horspool_algorithm
class BMH_Searcher
  {
  private:
    ptrdiff_t m_plen;
    ptrdiff_t m_bcrs[0x100];

  public:
    template<typename IterT>
    BMH_Searcher(IterT pbegin, IterT pend)
      {
        // Calculate the pattern length.
        this->m_plen = ::std::distance(pbegin, pend);

        // Build a table according to the Bad Character Rule.
        for(size_t i = 0;  i < 0x100;  ++i)
          this->m_bcrs[i] = this->m_plen;

        for(ptrdiff_t i = this->m_plen - 1;  i != 0;  --i)
          this->m_bcrs[uint8_t(pend[~i])] = i;
      }

  public:
    ptrdiff_t
    pattern_length()
    const noexcept
      { return this->m_plen;  }

    template<typename IterT>
    opt<IterT>
    search_opt(IterT tbegin, IterT tend, IterT pbegin)
    const
      {
        // Search for the pattern.
        auto tcur = tbegin;
        for(;;) {
          if(tend - tcur < this->m_plen)
            return nullopt;
          auto tnext = tcur + this->m_plen;
          if(::std::equal(tcur, tnext, pbegin))
            return tcur;
          // Adjust the read iterator using the Bad Character Rule.
          tcur += this->m_bcrs[uint8_t(tnext[-1])];
        }
      }
  };

template<typename IterT>
opt<IterT>
do_find_opt(IterT tbegin, IterT tend, IterT pbegin, IterT pend)
  {
    // If the pattern is empty, there is a match at the beginning.
    if(pbegin == pend)
      return tbegin;

    // If the text is empty but the pattern is not, there cannot be matches.
    if(tbegin == tend)
      return nullopt;

    // This is the slow path.
    BMH_Searcher srch(pbegin, pend);
    return srch.search_opt(tbegin, tend, pbegin);
  }

template<typename IterT>
V_string&
do_find_and_replace(V_string& res, IterT tbegin, IterT tend, IterT pbegin, IterT pend,
                    IterT rbegin, IterT rend)
  {
    // If the pattern is empty, there is a match beside every byte.
    if(pbegin == pend) {
      // This is really evil.
      for(auto it = tbegin;  it != tend;  ++it) {
        res.append(rbegin, rend);
        res.push_back(*it);
      }
      res.append(rbegin, rend);
      return res;
    }

    // If the text is empty but the pattern is not, there cannot be matches.
    if(tbegin == tend)
      return res;

    // This is the slow path.
    BMH_Searcher srch(pbegin, pend);
    auto tcur = tbegin;
    for(;;) {
      auto qtnext = srch.search_opt(tcur, tend, pbegin);
      if(!qtnext) {
        // Append all remaining characters and finish.
        res.append(tcur, tend);
        break;
      }

      // Append all characters that precede the match, followed by the replacement string.
      res.append(tcur, *qtnext);
      res.append(rbegin, rend);

      // Move `tcur` past the match.
      tcur = *qtnext + srch.pattern_length();
    }
    return res;
  }

template<typename IterT>
opt<IterT> do_find_of_opt(IterT begin, IterT end, const V_string& set, bool match)
  {
    // Make a lookup table.
    array<bool, 256> table = { };
    ::rocket::for_each(set, [&](char c) { table[uint8_t(c)] = true;  });

    // Search the range.
    for(auto it = begin;  it != end;  ++it)
      if(table[uint8_t(*it)] == match)
        return ::std::move(it);
    return nullopt;
  }

V_string do_get_reject(const optV_string& reject)
  {
    if(!reject)
      return ::rocket::sref(" \t");

    return *reject;
  }

V_string do_get_padding(const optV_string& padding)
  {
    if(!padding)
      return ::rocket::sref(" ");

    if(padding->empty())
      ASTERIA_THROW("Empty padding string not valid");

    return *padding;
  }

// These are strings of single characters.
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

// http://www.faqs.org/rfcs/rfc3986.html
// * Bit 0 indicates whether the character is a reserved character.
// * Bit 1 indicates whether the character is allowed unencoded in queries.
constexpr char s_url_chars[256] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 3, 0, 1, 3, 0, 3, 3, 3, 3, 3, 1, 3, 2, 2, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 0, 1, 0, 3,
    3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 1, 0, 2,
    0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };

constexpr
bool
do_is_url_invalid_char(char c)
noexcept
  { return s_url_chars[uint8_t(c)] == 0;  }

constexpr
bool
do_is_url_unreserved_char(char c)
noexcept
  { return s_url_chars[uint8_t(c)] == 2;  }

constexpr
bool
do_is_url_query_char(char c)
noexcept
  { return s_url_chars[uint8_t(c)] & 2;  }

const char*
do_xstrchr(const char* str, char c)
noexcept
  {
    // If `c == 0`, this function returns a null pointer.
    for(auto p = str;  *p != 0;  ++p)
      if(*p == c)
        return p;
    return nullptr;
  }

template<bool queryT>
V_string
do_url_encode(const V_string& data, bool lcase)
  {
    V_string text = data;
    // Only modify the string as needed, without causing copies on write.
    size_t nread = 0;
    while(nread != text.size()) {
      // Check whether this character has no special meaning.
      char c = text[nread++];
      if(queryT) {
        // This is the only special case.
        if(c == ' ') {
          text.mut(nread - 1) = '+';
          continue;
        }
        if(do_is_url_query_char(c))
          continue;
      }
      else {
        if(do_is_url_unreserved_char(c))
          continue;
      }

      // Escape it.
      char rep[3] = { '%', s_base16_table[((c >> 3) & 0x1E) + lcase],
                           s_base16_table[((c << 1) & 0x1E) + lcase] };

      // Replace this character with the escape string.
      text.replace(nread - 1, 1, rep, 3);
      nread += 2;
    }
    return text;
  }

template<bool queryT>
V_string do_url_decode(const V_string& text)
  {
    V_string data = text;
    // Only modify the string as needed, without causing copies on write.
    size_t nread = 0;
    while(nread != data.size()) {
      // Look for a character.
      char c = data[nread++];
      if(queryT) {
        // This is the only special case.
        if(c == '+') {
          data.mut(nread - 1) = ' ';
          continue;
        }
      }
      if(c != '%') {
        if(do_is_url_invalid_char(c))
          ASTERIA_THROW("Invalid character in URL (character `$1`)", c);

        continue;
      }

      // Two hexadecimal characters shall follow.
      if(data.size() - nread < 2)
        ASTERIA_THROW("No enough hexadecimal digits after `%`");

      // Parse the first digit.
      c = data[nread++];
      const char* pos = do_xstrchr(s_base16_table, c);
      if(!pos)
        ASTERIA_THROW("Invalid hexadecimal digit (character `$1`)", c);
      uint32_t reg = static_cast<uint32_t>(pos - s_base16_table) / 2 * 16;

      // Parse the second digit.
      c = data[nread++];
      pos = do_xstrchr(s_base16_table, c);
      if(!pos)
        ASTERIA_THROW("Invalid hexadecimal digit (character `$1`)", c);
      reg |= static_cast<uint32_t>(pos - s_base16_table) / 2;

      // Replace this sequence with the decoded byte.
      data.replace(nread - 3, 3, 1, static_cast<char>(reg));
      nread -= 2;
    }
    return data;
  }

template<bool bigendT>
struct Bswap;

template<>
struct Bswap<true>  // big endian
  {
    static inline
    int64_t
    conv(int64_t word)
    noexcept
      { return int64_t(be64toh(uint64_t(word)));  }

    static inline
    int32_t
    conv(int32_t word)
    noexcept
      { return int32_t(be32toh(uint32_t(word)));  }

    static inline
    int16_t
    conv(int16_t word)
    noexcept
      { return int16_t(be16toh(uint16_t(word)));  }

    static inline
    int8_t
    conv(int8_t word)
    noexcept
      { return word;  }

    static inline
    int
    conv(...)
    noexcept
      = delete;
  };

template<>
struct Bswap<false>  // little endian
  {
    static inline
    int64_t
    conv(int64_t word)
    noexcept
      { return int64_t(le64toh(uint64_t(word)));  }

    static inline
    int32_t
    conv(int32_t word)
    noexcept
      { return int32_t(le32toh(uint32_t(word)));  }

    static inline
    int16_t
    conv(int16_t word)
    noexcept
      { return int16_t(le16toh(uint16_t(word)));  }

    static inline
    int8_t
    conv(int8_t word)
    noexcept
      { return word;  }

    static inline
    int
    conv(...)
    noexcept
      = delete;
  };

template<bool bigendT, typename WordT>
V_string&
do_pack_one_impl(V_string& text, const V_integer& value)
  {
    // Convert the value into the specified byte order.
    auto word = Bswap<bigendT>::conv(static_cast<WordT>(value));
    text.append((char (&)[])word, sizeof(word));
    return text;
  }

template<typename WordT>
V_string&
do_pack_one_be(V_string& text, const V_integer& value)
  {
    return do_pack_one_impl<1, WordT>(text, value);
  }

template<typename WordT>
V_string&
do_pack_one_le(V_string& text, const V_integer& value)
  {
    return do_pack_one_impl<0, WordT>(text, value);
  }

template<bool bigendT, typename WordT>
V_array
do_unpack_impl(const V_string& text)
  {
    // How many words will the result have?
    auto nwords = text.size() / sizeof(WordT);
    if(text.size() != nwords * sizeof(WordT))
      ASTERIA_THROW("Invalid source string length (`$1` not divisible by `$2`)",
                    text.size(), sizeof(WordT));
    const auto pwords = reinterpret_cast<const WordT*>(text.data());

    V_array values;
    values.reserve(nwords);

    // Unpack integers.
    for(size_t i = 0;  i < nwords;  ++i) {
      auto word = Bswap<bigendT>::conv(pwords[i]);
      values.emplace_back(V_integer(word));
    }
    return values;
  }

template<typename WordT>
V_array
do_unpack_be(const V_string& text)
  {
    return do_unpack_impl<1, WordT>(text);
  }

template<typename WordT>
V_array
do_unpack_le(const V_string& text)
  {
    return do_unpack_impl<0, WordT>(text);
  }

::std::regex
do_make_regex(const V_string& pattern)
  try {
    return ::std::regex(pattern.data(), pattern.size());
  }
  catch(::std::regex_error& stdex) {
    ASTERIA_THROW("Invalid regular expression (text `$1`): $2", pattern, stdex.what());
  }

::std::string
do_make_regex_replacement(const V_string& replacement)
  {
    return ::std::string(replacement.data(), replacement.size());
  }

template<typename IterT>
::std::sub_match<IterT>
do_regex_search(IterT tbegin, IterT tend, const ::std::regex& pattern)
  {
    ::std::sub_match<IterT> match;
    ::std::match_results<IterT> matches;
    if(::std::regex_search(tbegin, tend, matches, pattern))
      match = matches[0];
    return match;
  }

template<typename IterT>
::std::match_results<IterT>
do_regex_match(IterT tbegin, IterT tend, const ::std::regex& pattern)
  {
    ::std::match_results<IterT> matches;
    ::std::regex_match(tbegin, tend, matches, pattern);
    return matches;
  }

template<typename IterT>
V_array
do_regex_copy_matches(const ::std::match_results<IterT>& matches)
  {
    V_array rval(matches.size());
    for(size_t i = 0;  i < matches.size();  ++i) {
      const auto& m = matches[i];
      if(m.matched)
        rval.mut(i) = V_string(m.first, m.second);
    }
    return rval;
  }

template<typename IterT>
V_string&
do_regex_replace(V_string& res, IterT tbegin, IterT tend,
                 const ::std::regex& pattern, const ::std::string& replacement)
  {
    ::std::regex_replace(::std::back_inserter(res), tbegin, tend, pattern, replacement);
    return res;
  }

}  // namespace

V_string
std_string_slice(V_string text, V_integer from, optV_integer length)
  {
    auto range = do_slice(text, from, length);
    if((range.first == text.begin()) && (range.second == text.end()))
      // Use reference counting as our advantage.
      return text;
    return V_string(range.first, range.second);
  }

V_string
std_string_replace_slice(V_string text, V_integer from, optV_integer length, V_string replacement)
  {
    V_string res = text;
    auto range = do_slice(res, from, length);
    // Replace the subrange.
    res.replace(range.first, range.second, replacement);
    return res;
  }

V_integer
std_string_compare(V_string text1, V_string text2, optV_integer length)
  {
    if(!length || (*length >= PTRDIFF_MAX))
      // Compare the entire strings.
      return text1.compare(text2);

    if(*length <= 0)
      // There is nothing to compare.
      return 0;

    // Compare two substrings.
    return text1.compare(0, static_cast<size_t>(*length), text2, 0, static_cast<size_t>(*length));
  }

V_boolean
std_string_starts_with(V_string text, V_string prefix)
  {
    return text.starts_with(prefix);
  }

V_boolean
std_string_ends_with(V_string text, V_string suffix)
  {
    return text.ends_with(suffix);
  }

optV_integer
std_string_find(V_string text, V_integer from, optV_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
    if(!qit)
      return nullopt;

    return *qit - text.begin();
  }

optV_integer
std_string_rfind(V_string text, V_integer from, optV_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second), ::std::make_reverse_iterator(range.first),
                           pattern.rbegin(), pattern.rend());
    if(!qit)
      return nullopt;

    return text.rend() - *qit - pattern.ssize();
  }

V_string
std_string_find_and_replace(V_string text, V_integer from, optV_integer length, V_string pattern,
                            V_string replacement)
  {
    V_string res;
    auto range = do_slice(text, from, length);
    res.append(text.begin(), range.first);
    do_find_and_replace(res, range.first, range.second, pattern.begin(), pattern.end(),
                             replacement.begin(), replacement.end());
    res.append(range.second, text.end());
    return res;
  }

optV_integer
std_string_find_any_of(V_string text, V_integer from, optV_integer length, V_string accept)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(range.first, range.second, accept, true);
    if(!qit)
      return nullopt;

    return *qit - text.begin();
  }

optV_integer
std_string_find_not_of(V_string text, V_integer from, optV_integer length, V_string reject)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(range.first, range.second, reject, false);
    if(!qit)
      return nullopt;

    return *qit - text.begin();
  }

optV_integer
std_string_rfind_any_of(V_string text, V_integer from, optV_integer length, V_string accept)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(::std::make_reverse_iterator(range.second),
                              ::std::make_reverse_iterator(range.first), accept, true);
    if(!qit)
      return nullopt;

    return text.rend() - *qit - 1;
  }

optV_integer
std_string_rfind_not_of(V_string text, V_integer from, optV_integer length, V_string reject)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(::std::make_reverse_iterator(range.second),
                              ::std::make_reverse_iterator(range.first), reject, false);
    if(!qit)
      return nullopt;

    return text.rend() - *qit - 1;
  }

V_string
std_string_reverse(V_string text)
  {
    // This is an easy matter, isn't it?
    return V_string(text.rbegin(), text.rend());
  }

V_string
std_string_trim(V_string text, optV_string reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0)
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Get the index of the first byte to keep.
    size_t bpos = text.find_first_not_of(rchars);
    if(bpos == V_string::npos)
      // There is no byte to keep. Return an empty string.
      return nullopt;

    // Get the index of the last byte to keep.
    size_t epos = text.find_last_not_of(rchars) + 1;
    if((bpos == 0) && (epos == text.size()))
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Return the remaining part of `text`.
    return text.substr(bpos, epos - bpos);
  }

V_string
std_string_triml(V_string text, optV_string reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0)
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Get the index of the first byte to keep.
    size_t bpos = text.find_first_not_of(rchars);
    if(bpos == V_string::npos)
      // There is no byte to keep. Return an empty string.
      return nullopt;

    if(bpos == 0)
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Return the remaining part of `text`.
    return text.substr(bpos);
  }

V_string
std_string_trimr(V_string text, optV_string reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0)
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Get the index of the last byte to keep.
    size_t epos = text.find_last_not_of(rchars) + 1;
    if(epos == 0)
      // There is no byte to keep. Return an empty string.
      return nullopt;

    if(epos == text.size())
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Return the remaining part of `text`.
    return text.substr(0, epos);
  }

V_string
std_string_padl(V_string text, V_integer length, optV_string padding)
  {
    V_string res = text;
    auto rpadding = do_get_padding(padding);
    if(length <= 0)
      // There is nothing to do.
      return res;

    // Fill `rpadding` at the front.
    res.reserve(static_cast<size_t>(length));
    while(res.size() + rpadding.length() <= static_cast<uint64_t>(length))
      res.insert(res.end() - text.ssize(), rpadding);
    return res;
  }

V_string
std_string_padr(V_string text, V_integer length, optV_string padding)
  {
    V_string res = text;
    auto rpadding = do_get_padding(padding);
    if(length <= 0)
      // There is nothing to do.
      return res;

    // Fill `rpadding` at the back.
    res.reserve(static_cast<size_t>(length));
    while(res.size() + rpadding.length() <= static_cast<uint64_t>(length))
      res.append(rpadding);
    return res;
  }

V_string
std_string_to_upper(V_string text)
  {
    V_string res = text;
    char* wptr = nullptr;

    // Translate each character.
    for(size_t i = 0;  i < res.size();  ++i) {
      char c = res[i];
      if((c < 'a') || ('z' < c))
        continue;

      // Fork the string as needed.
      if(ROCKET_UNEXPECT(!wptr))
        wptr = res.mut_data();

      wptr[i] = static_cast<char>(c - 'a' + 'A');
    }
    return res;
  }

V_string
std_string_to_lower(V_string text)
  {
    V_string res = text;
    char* wptr = nullptr;

    // Translate each character.
    for(size_t i = 0;  i < res.size();  ++i) {
      char c = res[i];
      if((c < 'A') || ('Z' < c))
        continue;

      // Fork the string as needed.
      if(ROCKET_UNEXPECT(!wptr))
        wptr = res.mut_data();

      wptr[i] = static_cast<char>(c - 'A' + 'a');
    }
    return res;
  }

V_string
std_string_translate(V_string text, V_string inputs, optV_string outputs)
  {
    // Use reference counting as our advantage.
    V_string res = text;
    char* wptr = nullptr;

    // Translate each character.
    for(size_t i = 0;  i < res.size();  ++i) {
      char c = res[i];
      auto ipos = inputs.find(c);
      if(ipos == V_string::npos)
        continue;

      // Fork the string as needed.
      if(ROCKET_UNEXPECT(!wptr))
        wptr = res.mut_data();

      if(!outputs || (ipos >= outputs->size()))
        // Erase the byte if there is no replacement.
        // N.B. This must cause no reallocation.
        res.erase(i--, 1);
      else
        // Replace the character.
        wptr[i] = outputs->data()[ipos];
    }
    return res;
  }

V_array
std_string_explode(V_string text, optV_string delim, optV_integer limit)
  {
    uint64_t rlimit = UINT64_MAX;
    if(limit) {
      if(*limit <= 0)
        ASTERIA_THROW("Max number of segments must be positive (limit `$1`)", *limit);
      rlimit = static_cast<uint64_t>(*limit);
    }

    V_array segments;
    if(text.empty())
      // Return an empty array.
      return segments;

    if(!delim || delim->empty()) {
      // Split every byte.
      segments.reserve(text.size());
      for(size_t i = 0;  i < text.size();  ++i) {
        uint32_t b = text[i] & 0xFF;
        // Store a reference to the null-terminated string allocated statically.
        // Don't bother allocating a new buffer of only two characters.
        segments.emplace_back(V_string(::rocket::sref(s_char_table[b], 1)));
      }
      return segments;
    }

    // Break `text` down.
    auto bpos = text.begin();
    auto epos = text.end();
    for(;;) {
      if(segments.size() + 1 >= rlimit) {
        segments.emplace_back(V_string(bpos, epos));
        break;
      }
      auto mpos = ::std::search(bpos, epos, delim->begin(), delim->end());
      if(mpos == epos) {
        segments.emplace_back(V_string(bpos, epos));
        break;
      }
      segments.emplace_back(V_string(bpos, mpos));
      bpos = mpos + delim->ssize();
    }
    return segments;
  }

V_string
std_string_implode(V_array segments, optV_string delim)
  {
    V_string text;
    auto nsegs = segments.size();
    if(nsegs == 0)
      // Return an empty string.
      return text;

    // Append the first string.
    text = segments.front().as_string();
    // Any segment other than the first one follows a delimiter.
    for(size_t i = 1;  i != nsegs;  ++i) {
      if(delim)
        text += *delim;
      text += segments[i].as_string();
    }
    return text;
  }

V_string
std_string_hex_encode(V_string data, optV_boolean lowercase, optV_string delim)
  {
    V_string text;
    auto rdelim = delim ? ::rocket::sref(*delim) : ::rocket::sref("");
    bool rlowerc = lowercase.value_or(false);
    text.reserve(data.size() * (2 + rdelim.length()));

    // These shall be operated in big-endian order.
    uint32_t reg = 0;

    // Encode source data.
    size_t nread = 0;
    while(nread != data.size()) {
      // Insert a delimiter before every byte other than the first one.
      if(!text.empty())
        text += rdelim;

      // Read a byte.
      reg = data[nread++] & 0xFF;
      reg <<= 24;

      // Encode it.
      for(size_t i = 0;  i < 2;  ++i) {
        uint32_t b = ((reg >> 28) * 2 + rlowerc) & 0xFF;
        reg <<= 4;
        text += s_base16_table[b];
      }
    }
    return text;
  }

V_string
std_string_hex_decode(V_string text)
  {
    V_string data;

    // These shall be operated in big-endian order.
    uint32_t reg = 1;

    // Decode source data.
    size_t nread = 0;
    while(nread != text.size()) {
      // Read and identify a character.
      char c = text[nread++];
      const char* pos = do_xstrchr(s_spaces, c);
      if(pos) {
        // The character is a whitespace.
        if(reg != 1)
          ASTERIA_THROW("Unpaired hexadecimal digit");

        continue;
      }
      reg <<= 4;

      // Decode a digit.
      pos = do_xstrchr(s_base16_table, c);
      if(!pos)
        ASTERIA_THROW("Invalid hexadecimal digit (character `$1`)", c);
      reg |= static_cast<uint32_t>(pos - s_base16_table) / 2;

      // Decode the current group if it is complete.
      if(!(reg & 0x1'00))
        continue;

      data += static_cast<char>(reg);
      reg = 1;
    }
    if(reg != 1)
      ASTERIA_THROW("Unpaired hexadecimal digit");
    return data;
  }

V_string
std_string_base32_encode(V_string data, optV_boolean lowercase)
  {
    V_string text;
    bool rlowerc = lowercase.value_or(false);
    text.reserve((data.size() + 4) / 5 * 8);

    // These shall be operated in big-endian order.
    uint64_t reg = 0;

    // Encode source data.
    size_t nread = 0;
    while(data.size() - nread >= 5) {
      // Read 5 consecutive bytes.
      for(size_t i = 0;  i < 5;  ++i) {
        uint32_t b = data[nread++] & 0xFF;
        reg <<= 8;
        reg |= b;
      }
      reg <<= 24;

      // Encode them.
      for(size_t i = 0;  i < 8;  ++i) {
        uint32_t b = ((reg >> 59) * 2 + rlowerc) & 0xFF;
        reg <<= 5;
        text += s_base32_table[b];
      }
    }
    if(nread != data.size()) {
      // Get the start of padding characters.
      size_t m = data.size() - nread;
      size_t p = (m * 8 + 4) / 5;

      // Read all remaining bytes that cannot fill up a unit.
      for(size_t i = 0;  i < m;  ++i) {
        uint32_t b = data[nread++] & 0xFF;
        reg <<= 8;
        reg |= b;
      }
      reg <<= 64 - m * 8;

      // Encode them.
      for(size_t i = 0;  i < p;  ++i) {
        uint32_t b = ((reg >> 59) * 2 + rlowerc) & 0xFF;
        reg <<= 5;
        text += s_base32_table[b];
      }

      // Fill padding characters.
      for(size_t i = p;  i != 8;  ++i)
        text += s_base32_table[64];
    }
    return text;
  }

V_string
std_string_base32_decode(V_string text)
  {
    V_string data;

    // These shall be operated in big-endian order.
    uint64_t reg = 1;
    uint32_t npad = 0;

    // Decode source data.
    size_t nread = 0;
    while(nread != text.size()) {
      // Read and identify a character.
      char c = text[nread++];
      const char* pos = do_xstrchr(s_spaces, c);
      if(pos) {
        // The character is a whitespace.
        if(reg != 1)
          ASTERIA_THROW("Incomplete base32 group");

        continue;
      }
      reg <<= 5;

      if(c == s_base32_table[64]) {
        // The character is a padding character.
        if(reg < 0x100)
          ASTERIA_THROW("Unexpected base32 padding character");

        npad += 1;
      }
      else {
        // Decode a digit.
        pos = do_xstrchr(s_base32_table, c);
        if(!pos)
          ASTERIA_THROW("Invalid base32 digit (character `$1`)", c);

        if(npad != 0)
          ASTERIA_THROW("Unexpected base32 digit following padding character");

        reg |= static_cast<uint32_t>(pos - s_base32_table) / 2;
      }

      // Decode the current group if it is complete.
      if(!(reg & 0x1'00'00'00'00'00))
        continue;

      size_t m = (40 - npad * 5) / 8;
      size_t p = (m * 8 + 4) / 5;
      if(p + npad != 8)
        ASTERIA_THROW("Unexpected number of base32 padding characters (got `$1`)", npad);

      for(size_t i = 0; i < m; ++i) {
        reg <<= 8;
        data += static_cast<char>(reg >> 40);
      }
      reg = 1;
      npad = 0;
    }
    if(reg != 1)
      ASTERIA_THROW("Incomplete base32 group");
    return data;
  }

V_string
std_string_base64_encode(V_string data)
  {
    V_string text;
    text.reserve((data.size() + 2) / 3 * 4);

    // These shall be operated in big-endian order.
    uint32_t reg = 0;

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
        uint32_t b = (reg >> 26) & 0xFF;
        reg <<= 6;
        text += s_base64_table[b];
      }
    }
    if(nread != data.size()) {
      // Get the start of padding characters.
      size_t m = data.size() - nread;
      size_t p = (m * 8 + 5) / 6;

      // Read all remaining bytes that cannot fill up a unit.
      for(size_t i = 0;  i < m;  ++i) {
        uint32_t b = data[nread++] & 0xFF;
        reg <<= 8;
        reg |= b;
      }
      reg <<= 32 - m * 8;

      // Encode them.
      for(size_t i = 0;  i < p;  ++i) {
        uint32_t b = (reg >> 26) & 0xFF;
        reg <<= 6;
        text += s_base64_table[b];
      }

      // Fill padding characters.
      for(size_t i = p;  i != 4;  ++i)
        text += s_base64_table[64];
    }
    return text;
  }

V_string
std_string_base64_decode(V_string text)
  {
    V_string data;

    // These shall be operated in big-endian order.
    uint32_t reg = 1;
    uint32_t npad = 0;

    // Decode source data.
    size_t nread = 0;
    while(nread != text.size()) {
      // Read and identify a character.
      char c = text[nread++];
      const char* pos = do_xstrchr(s_spaces, c);
      if(pos) {
        // The character is a whitespace.
        if(reg != 1)
          ASTERIA_THROW("Incomplete base64 group");

        continue;
      }
      reg <<= 6;

      if(c == s_base64_table[64]) {
        // The character is a padding character.
        if(reg < 0x100)
          ASTERIA_THROW("Unexpected base64 padding character");

        npad += 1;
      }
      else {
        // Decode a digit.
        pos = do_xstrchr(s_base64_table, c);
        if(!pos)
          ASTERIA_THROW("Invalid base64 digit (character `$1`)", c);

        if(npad != 0)
          ASTERIA_THROW("Unexpected base64 digit following padding character");

        reg |= static_cast<uint32_t>(pos - s_base64_table);
      }

      // Decode the current group if it is complete.
      if(!(reg & 0x1'00'00'00))
        continue;

      size_t m = (24 - npad * 6) / 8;
      size_t p = (m * 8 + 5) / 6;
      if(p + npad != 4)
        ASTERIA_THROW("Unexpected number of base64 padding characters (got `$1`)", npad);

      for(size_t i = 0; i < m; ++i) {
        reg <<= 8;
        data += static_cast<char>(reg >> 24);
      }
      reg = 1;
      npad = 0;
    }
    if(reg != 1)
      ASTERIA_THROW("Incomplete base64 group");
    return data;
  }

V_string
std_string_url_encode(V_string data, optV_boolean lowercase)
  {
    return do_url_encode<0>(data, lowercase.value_or(false));
  }

V_string
std_string_url_decode(V_string text)
  {
    return do_url_decode<0>(text);
  }

V_string
std_string_url_encode_query(V_string data, optV_boolean lowercase)
  {
    return do_url_encode<1>(data, lowercase.value_or(false));
  }

V_string
std_string_url_decode_query(V_string text)
  {
    return do_url_decode<1>(text);
  }

V_boolean
std_string_utf8_validate(V_string text)
  {
    size_t offset = 0;
    while(offset < text.size()) {
      // Try decoding a code point.
      char32_t cp;
      if(!utf8_decode(cp, text, offset))
        // This sequence is invalid.
        return false;
    }
    return true;
  }

V_string
std_string_utf8_encode(V_integer code_point, optV_boolean permissive)
  {
    V_string text;
    text.reserve(4);

    // Try encoding the code point.
    auto cp = static_cast<char32_t>(::rocket::clamp(code_point, -1, INT32_MAX));
    if(!utf8_encode(text, cp)) {
      // This comparison with `true` is by intention, because it may be unset.
      if(permissive != true)
        ASTERIA_THROW("Invalid UTF code point (value `$1`)", code_point);
      utf8_encode(text, 0xFFFD);
    }
    return text;
  }

V_string
std_string_utf8_encode(V_array code_points, optV_boolean permissive)
  {
    V_string text;
    text.reserve(code_points.size() * 3);
    for(const auto& elem : code_points) {
      // Try encoding the code point.
      V_integer value = elem.as_integer();
      auto cp = static_cast<char32_t>(::rocket::clamp(value, -1, INT32_MAX));
      if(!utf8_encode(text, cp)) {
        // This comparison with `true` is by intention, because it may be unset.
        if(permissive != true)
          ASTERIA_THROW("Invalid UTF code point (value `$1`)", value);
        utf8_encode(text, 0xFFFD);
      }
    }
    return text;
  }

V_array
std_string_utf8_decode(V_string text, optV_boolean permissive)
  {
    V_array code_points;
    code_points.reserve(text.size());

    size_t offset = 0;
    while(offset < text.size()) {
      // Try decoding a code point.
      char32_t cp;
      if(!utf8_decode(cp, text, offset)) {
        // This comparison with `true` is by intention, because it may be unset.
        if(permissive != true)
          ASTERIA_THROW("Invalid UTF-8 string");
        cp = text[offset++] & 0xFF;
      }
      code_points.emplace_back(V_integer(cp));
    }
    return code_points;
  }

V_string
std_string_pack_8(V_integer value)
  {
    V_string text;
    text.reserve(1);
    do_pack_one_le<int8_t>(text, value);
    return text;
  }

V_string
std_string_pack_8(V_array values)
  {
    V_string text;
    text.reserve(values.size());
    for(const auto& elem : values)
      do_pack_one_le<int8_t>(text, elem.as_integer());
    return text;
  }

V_array
std_string_unpack_8(V_string text)
  {
    return do_unpack_le<int8_t>(text);
  }

V_string
std_string_pack_16be(V_integer value)
  {
    V_string text;
    text.reserve(2);
    do_pack_one_be<int16_t>(text, value);
    return text;
  }

V_string
std_string_pack_16be(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 2);
    for(const auto& elem : values)
      do_pack_one_be<int16_t>(text, elem.as_integer());
    return text;
  }

V_array
std_string_unpack_16be(V_string text)
  {
    return do_unpack_be<int16_t>(text);
  }

V_string
std_string_pack_16le(V_integer value)
  {
    V_string text;
    text.reserve(2);
    do_pack_one_le<int16_t>(text, value);
    return text;
  }

V_string
std_string_pack_16le(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 2);
    for(const auto& elem : values)
      do_pack_one_le<int16_t>(text, elem.as_integer());
    return text;
  }

V_array
std_string_unpack_16le(V_string text)
  {
    return do_unpack_le<int16_t>(text);
  }

V_string
std_string_pack_32be(V_integer value)
  {
    V_string text;
    text.reserve(4);
    do_pack_one_be<int32_t>(text, value);
    return text;
  }

V_string
std_string_pack_32be(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 4);
    for(const auto& elem : values)
      do_pack_one_be<int32_t>(text, elem.as_integer());
    return text;
  }

V_array
std_string_unpack_32be(V_string text)
  {
    return do_unpack_be<int32_t>(text);
  }

V_string
std_string_pack_32le(V_integer value)
  {
    V_string text;
    text.reserve(4);
    do_pack_one_le<int32_t>(text, value);
    return text;
  }

V_string
std_string_pack_32le(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 4);
    for(const auto& elem : values)
      do_pack_one_le<int32_t>(text, elem.as_integer());
    return text;
  }

V_array
std_string_unpack_32le(V_string text)
  {
    return do_unpack_le<int32_t>(text);
  }

V_string
std_string_pack_64be(V_integer value)
  {
    V_string text;
    text.reserve(8);
    do_pack_one_be<int64_t>(text, value);
    return text;
  }

V_string
std_string_pack_64be(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 8);
    for(const auto& elem : values)
      do_pack_one_be<int64_t>(text, elem.as_integer());
    return text;
  }

V_array
std_string_unpack_64be(V_string text)
  {
    return do_unpack_be<int64_t>(text);
  }

V_string
std_string_pack_64le(V_integer value)
  {
    V_string text;
    text.reserve(8);
    do_pack_one_le<int64_t>(text, value);
    return text;
  }

V_string
std_string_pack_64le(V_array values)
  {
    V_string text;
    text.reserve(values.size() * 8);
    for(const auto& elem : values)
      do_pack_one_le<int64_t>(text, elem.as_integer());
    return text;
  }

V_array
std_string_unpack_64le(V_string text)
  {
    return do_unpack_le<int64_t>(text);
  }

V_string
std_string_format(V_string templ, cow_vector<Value> values)
  {
    // Prepare inserters.
    cow_vector<::rocket::formatter> insts;
    insts.reserve(values.size());
    for(size_t i = 0;  i < values.size();  ++i)
      insts.push_back({
        [](tinyfmt& fmt, const void* ptr) -> tinyfmt&
          { return static_cast<const Value*>(ptr)->print(fmt);  },
        values.data() + i
      });

    // Compose the string into a stream.
    ::rocket::tinyfmt_str fmt;
    vformat(fmt, templ.data(), templ.size(), insts.data(), insts.size());
    return fmt.extract_string();
  }

opt<pair<V_integer, V_integer>>
std_string_regex_find(V_string text, V_integer from, optV_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);
    auto match = do_regex_search(range.first, range.second, do_make_regex(pattern));
    if(!match.matched)
      return nullopt;
    return ::std::make_pair(match.first - text.begin(), match.second - match.first);
  }

optV_array
std_string_regex_match(V_string text, V_integer from, optV_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);
    auto matches = do_regex_match(range.first, range.second, do_make_regex(pattern));
    if(matches.empty())
      return nullopt;
    return do_regex_copy_matches(matches);
  }

V_string
std_string_regex_replace(V_string text, V_integer from, optV_integer length, V_string pattern,
                         V_string replacement)
  {
    V_string res;
    auto range = do_slice(text, from, length);
    res.append(text.begin(), range.first);
    do_regex_replace(res, range.first, range.second, do_make_regex(pattern),
                          do_make_regex_replacement(replacement));
    res.append(range.second, text.end());
    return res;
  }

void
create_bindings_string(V_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.string.slice()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("slice"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.slice(text, from, [length])`

  * Copies a subrange of `text` to create a new byte string. Bytes
    are copied from `from` if it is non-negative, or from
    `countof(text) + from` otherwise. If `length` is set to an
    integer, no more than this number of bytes will be copied. If
    it is absent, all bytes from `from` to the end of `text` will
    be copied. If `from` is outside `text`, an empty string is
    returned.

  * Returns the specified substring of `text`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.slice"));
    // Parse arguments.
    V_string text;
    V_integer from;
    optV_integer length;
    if(reader.I().v(text).v(from).o(length).F()) {
      Reference_root::S_temporary xref = { std_string_slice(::std::move(text), from, length) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.replace_slice()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("replace_slice"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.replace_slice(text, from, replacement)`

  * Replaces all bytes from `from` to the end of `text` with
    `replacement` and returns the new byte string. If `from` is
    negative, it specifies an offset from the end of `text`. This
    function returns a new string without modifying `text`.

  * Returns a string with the subrange replaced.

`std.string.replace_slice(text, from, [length], replacement)`

  * Replaces a subrange of `text` with `replacement` to create a
    new byte string. `from` specifies the start of the subrange to
    replace. If `from` is negative, it specifies an offset from the
    end of `text`. `length` specifies the maximum number of bytes
    to replace. If it is set to `null`, this function is equivalent
    to `replace_slice(text, from, replacement)`. This function
    returns a new string without modifying `text`.

  * Returns a string with the subrange replaced.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.replace"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_integer from;
    V_string replacement;
    if(reader.I().v(text).v(from).S(state).v(replacement).F()) {
      Reference_root::S_temporary xref = { std_string_replace_slice(::std::move(text), from, nullopt,
                                                                    ::std::move(replacement)) };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(replacement).F()) {
      Reference_root::S_temporary xref = { std_string_replace_slice(::std::move(text), from, length,
                                                                    ::std::move(replacement)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.compare()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("compare"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.compare(text1, text2, [length])`

  * Performs lexicographical comparison on two byte strings. If
    `length` is set to an integer, no more than this number of
    bytes are compared. This function behaves like the `strncmp()`
    function in C, except that null characters do not terminate
    strings.

  * Returns a positive integer if `text1` compares greater than
    `text2`, a negative integer if `text1` compares less than
    `text2`, or zero if `text1` compares equal to `text2`.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.compare"));
    // Parse arguments.
    V_string text1;
    V_string text2;
    optV_integer length;
    if(reader.I().v(text1).v(text2).o(length).F()) {
      Reference_root::S_temporary xref = { std_string_compare(::std::move(text1), ::std::move(text2), length) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.starts_with()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("starts_with"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.starts_with(text, prefix)`

  * Checks whether `prefix` is a prefix of `text`. The empty
    string is considered to be a prefix of any string.

  * Returns `true` if `prefix` is a prefix of `text`, or `false`
    otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.starts_with"));
    // Parse arguments.
    V_string text;
    V_string prefix;
    if(reader.I().v(text).v(prefix).F()) {
      Reference_root::S_temporary xref = { std_string_starts_with(::std::move(text), ::std::move(prefix)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.ends_with()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("ends_with"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.ends_with(text, suffix)`

  * Checks whether `suffix` is a suffix of `text`. The empty
    string is considered to be a suffix of any string.

  * Returns `true` if `suffix` is a suffix of `text`, or `false`
    otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.ends_with"));
    // Parse arguments.
    V_string text;
    V_string suffix;
    if(reader.I().v(text).v(suffix).F()) {
      Reference_root::S_temporary xref = { std_string_ends_with(::std::move(text), ::std::move(suffix)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.find()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.find(text, pattern)`

  * Searches `text` for the first occurrence of `pattern`.

  * Returns the subscript of the first byte of the first match of
    `pattern` in `text` if one is found, which is always
    non-negative, or `null` otherwise.

`std.string.find(text, from, pattern)`

  * Searches `text` for the first occurrence of `pattern`. The
    search operation is performed on the same subrange that would
    be returned by `slice(text, from)`.

  * Returns the subscript of the first byte of the first match of
    `pattern` in `text` if one is found, which is always
    non-negative, or `null` otherwise.

`std.string.find(text, from, [length], pattern)`

  * Searches `text` for the first occurrence of `pattern`. The
    search operation is performed on the same subrange that would
    be returned by `slice(text, from, length)`.

  * Returns the subscript of the first byte of the first match of
    `pattern` in `text` if one is found, which is always
    non-negative, or `null` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.find"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_string pattern;
    if(reader.I().v(text).S(state).v(pattern).F()) {
      Reference_root::S_temporary xref = { std_string_find(::std::move(text), 0, nullopt,
                                                           ::std::move(pattern)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(pattern).F()) {
      Reference_root::S_temporary xref = { std_string_find(::std::move(text), from, nullopt,
                                                           ::std::move(pattern)) };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(pattern).F()) {
      Reference_root::S_temporary xref = { std_string_find(::std::move(text), from, length,
                                                           ::std::move(pattern)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.rfind()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.rfind(text, pattern)`

  * Searches `text` for the last occurrence of `pattern`.

  * Returns the subscript of the first byte of the last match of
    `pattern` in `text` if one is found, which is always
    non-negative, or `null` otherwise.

`std.string.rfind(text, from, pattern)`

  * Searches `text` for the last occurrence of `pattern`. The
    search operation is performed on the same subrange that would
    be returned by `slice(text, from)`.

  * Returns the subscript of the first byte of the last match of
    `pattern` in `text` if one is found, which is always
    non-negative, or `null` otherwise.

`std.string.rfind(text, from, [length], pattern)`

  * Searches `text` for the last occurrence of `pattern`.

  * Returns the subscript of the first byte of the last match of
    `pattern` in `text` if one is found, which is always
    non-negative, or `null` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.rfind"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_string pattern;
    if(reader.I().v(text).S(state).v(pattern).F()) {
      Reference_root::S_temporary xref = { std_string_rfind(::std::move(text), 0, nullopt,
                                                            ::std::move(pattern)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(pattern).F()) {
      Reference_root::S_temporary xref = { std_string_rfind(::std::move(text), from, nullopt,
                                                            ::std::move(pattern)) };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(pattern).F()) {
      Reference_root::S_temporary xref = { std_string_rfind(::std::move(text), from, length,
                                                            ::std::move(pattern)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.find_and_replace()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_and_replace"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.find_and_replace(text, pattern, replacement)`

  * Searches `text` and replaces all occurrences of `pattern` with
    `replacement`. This function returns a new string without
    modifying `text`.

  * Returns the string with `pattern` replaced. If `text` does not
    contain `pattern`, it is returned intact.

`std.string.find_and_replace(text, from, pattern, replacement)`

  * Searches `text` and replaces all occurrences of `pattern` with
    `replacement`. The search operation is performed on the same
    subrange that would be returned by `slice(text, from)`. This
    function returns a new string without modifying `text`.

  * Returns the string with `pattern` replaced. If `text` does not
    contain `pattern`, it is returned intact.

`std.string.find_and_replace(text, from, [length], pattern, replacement)`

  * Searches `text` and replaces all occurrences of `pattern` with
    `replacement`. The search operation is performed on the same
    subrange that would be returned by `slice(text, from, length)`.
    This function returns a new string without modifying `text`.

  * Returns the string with `pattern` replaced. If `text` does not
    contain `pattern`, it is returned intact.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.find_and_replace"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_string pattern;
    V_string replacement;
    if(reader.I().v(text).S(state).v(pattern).v(replacement).F()) {
      Reference_root::S_temporary xref = { std_string_find_and_replace(::std::move(text), 0, nullopt,
                                                           ::std::move(pattern), ::std::move(replacement)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(pattern).v(replacement).F()) {
      Reference_root::S_temporary xref = { std_string_find_and_replace(::std::move(text), from, nullopt,
                                                           ::std::move(pattern), ::std::move(replacement)) };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(pattern).v(replacement).F()) {
      Reference_root::S_temporary xref = { std_string_find_and_replace(::std::move(text), from, length,
                                                           ::std::move(pattern), ::std::move(replacement)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.find_any_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_any_of"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.find_any_of(text, accept)`

  * Searches `text` for bytes that exist in `accept`.

  * Returns the subscript of the first byte found, which is always
    non-negative; or `null` if no such byte exists.

`std.string.find_any_of(text, from, accept)`

  * Searches `text` for bytes that exist in `accept`. The search
    operation is performed on the same subrange that would be
    returned by `slice(text, from)`.

  * Returns the subscript of the first byte found, which is always
    non-negative; or `null` if no such byte exists.

`std.string.find_any_of(text, from, [length], accept)`

  * Searches `text` for bytes that exist in `accept`. The search
    operation is performed on the same subrange that would be
    returned by `slice(text, from, length)`.

  * Returns the subscript of the first byte found, which is always
    non-negative; or `null` if no such byte exists.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.find_any_of"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_string accept;
    if(reader.I().v(text).S(state).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_find_any_of(::std::move(text), 0, nullopt,
                                                                  ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_find_any_of(::std::move(text), from, nullopt,
                                                                  ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_find_any_of(::std::move(text), from, length,
                                                                  ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.rfind_any_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind_any_of"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.rfind_any_of(text, accept)`

  * Searches `text` for bytes that exist in `accept`.

  * Returns the subscript of the last byte found, which is always
    non-negative; or `null` if no such byte exists.

`std.string.rfind_any_of(text, from, accept)`

  * Searches `text` for bytes that exist in `accept`. The search
    operation is performed on the same subrange that would be
    returned by `slice(text, from)`.

  * Returns the subscript of the last byte found, which is always
    non-negative; or `null` if no such byte exists.

`std.string.rfind_any_of(text, from, [length], accept)`

  * Searches `text` for bytes that exist in `accept`. The search
    operation is performed on the same subrange that would be
    returned by `slice(text, from, length)`.

  * Returns the subscript of the last byte found, which is always
    non-negative; or `null` if no such byte exists.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.rfind_any_of"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_string accept;
    if(reader.I().v(text).S(state).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_rfind_any_of(::std::move(text), 0, nullopt,
                                                                   ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_rfind_any_of(::std::move(text), from, nullopt,
                                                                   ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_rfind_any_of(::std::move(text), from, length,
                                                                   ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.find_not_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("find_not_of"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.find_not_of(text, reject)`

  * Searches `text` for bytes that does not exist in `reject`.

  * Returns the subscript of the first byte found, which is always
    non-negative; or `null` if no such byte exists.

`std.string.find_not_of(text, from, reject)`

  * Searches `text` for bytes that does not exist in `reject`. The
    search operation is performed on the same subrange that would
    be returned by `slice(text, from)`.

  * Returns the subscript of the first byte found, which is always
    non-negative; or `null` if no such byte exists.

`std.string.find_not_of(text, from, [length], reject)`

  * Searches `text` for bytes that does not exist in `reject`. The
    search operation is performed on the same subrange that would
    be returned by `slice(text, from, length)`.

  * Returns the subscript of the first byte found, which is always
    non-negative; or `null` if no such byte exists.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.find_not_of"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_string accept;
    if(reader.I().v(text).S(state).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_find_not_of(::std::move(text), 0, nullopt,
                                                                  ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_find_not_of(::std::move(text), from, nullopt,
                                                                  ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_find_not_of(::std::move(text), from, length,
                                                                  ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.rfind_not_of()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("rfind_not_of"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.rfind_not_of(text, reject)`

  * Searches `text` for bytes that does not exist in `reject`.

  * Returns the subscript of the last byte found, which is always
    non-negative; or `null` if no such byte exists.

`std.string.rfind_not_of(text, from, reject)`

  * Searches `text` for bytes that does not exist in `reject`. The
    search operation is performed on the same subrange that would
    be returned by `slice(text, from)`.

  * Returns the subscript of the last byte found, which is always
    non-negative; or `null` if no such byte exists.

`std.string.rfind_not_of(text, from, [length], reject)`

  * Searches `text` for bytes that does not exist in `reject`. The
    search operation is performed on the same subrange that would
    be returned by `slice(text, from, length)`.

  * Returns the subscript of the last byte found, which is always
    non-negative; or `null` if no such byte exists.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.rfind_not_of"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_string accept;
    if(reader.I().v(text).S(state).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_rfind_not_of(::std::move(text), 0, nullopt,
                                                                   ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_rfind_not_of(::std::move(text), from, nullopt,
                                                                   ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(accept).F()) {
      Reference_root::S_temporary xref = { std_string_rfind_not_of(::std::move(text), from, length,
                                                                   ::std::move(accept)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.reverse()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("reverse"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.reverse(text)`

  * Reverses a byte string. This function returns a new string
    without modifying `text`.

  * Returns the reversed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.reverse"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_reverse(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.trim()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("trim"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.trim(text, [reject])`

  * Removes the longest prefix and suffix consisting solely bytes
    from `reject`. If `reject` is empty, no byte is removed. If
    `reject` is not specified, spaces and tabs are removed. This
    function returns a new string without modifying `text`.

  * Returns the trimmed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.trim"));
    // Parse arguments.
    V_string text;
    optV_string reject;
    if(reader.I().v(text).o(reject).F()) {
      Reference_root::S_temporary xref = { std_string_trim(::std::move(text), ::std::move(reject)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.triml()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("triml"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.triml(text, [reject])`

  * Removes the longest prefix consisting solely bytes from
    `reject`. If `reject` is empty, no byte is removed. If `reject`
    is not specified, spaces and tabs are removed. This function
    returns a new string without modifying `text`.

  * Returns the trimmed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.triml"));
    // Parse arguments.
    V_string text;
    optV_string reject;
    if(reader.I().v(text).o(reject).F()) {
      Reference_root::S_temporary xref = { std_string_triml(::std::move(text), ::std::move(reject)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.trimr()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("trimr"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.trimr(text, [reject])`

  * Removes the longest suffix consisting solely bytes from
    `reject`. If `reject` is empty, no byte is removed. If `reject`
    is not specified, spaces and tabs are removed. This function
    returns a new string without modifying `text`.

  * Returns the trimmed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.trimr"));
    // Parse arguments.
    V_string text;
    optV_string reject;
    if(reader.I().v(text).o(reject).F()) {
      Reference_root::S_temporary xref = { std_string_trimr(::std::move(text), ::std::move(reject)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.padl()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("padl"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.padl(text, length, [padding])`

  * Prepends `text` with `padding` repeatedly, until its length
    would exceed `length`. The default value of `padding` is a
    string consisting of a space. This function returns a new
    string without modifying `text`.

  * Returns the padded string.

  * Throws an exception if `padding` is empty.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.padl"));
    // Parse arguments.
    V_string text;
    V_integer length;
    optV_string padding;
    if(reader.I().v(text).v(length).o(padding).F()) {
      Reference_root::S_temporary xref = { std_string_padl(::std::move(text), length, ::std::move(padding)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.padr()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("padr"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.padr(text, length, [padding])`

  * Appends `text` with `padding` repeatedly, until its length
    would exceed `length`. The default value of `padding` is a
    string consisting of a space. This function returns a new
    string without modifying `text`.

  * Returns the padded string.

  * Throws an exception if `padding` is empty.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.padr"));
    // Parse arguments.
    V_string text;
    V_integer length;
    optV_string padding;
    if(reader.I().v(text).v(length).o(padding).F()) {
      Reference_root::S_temporary xref = { std_string_padr(::std::move(text), length, ::std::move(padding)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.to_upper()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("to_upper"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.to_upper(text)`

  * Converts all lowercase English letters in `text` to their
    uppercase counterparts. This function returns a new string
    without modifying `text`.

  * Returns a new string after the conversion.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.to_upper"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_to_upper(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.to_lower()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("to_lower"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.to_lower(text)`

  * Converts all uppercase English letters in `text` to their
    lowercase counterparts. This function returns a new string
    without modifying `text`.

  * Returns a new string after the conversion.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.to_lower"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_to_lower(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.translate()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("translate"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.translate(text, inputs, [outputs])`

  * Performs bytewise translation on the given string. For every
    byte in `text` that is also found in `inputs`, if there is a
    corresponding replacement byte in `outputs` with the same
    subscript, it is replaced with the latter; if no replacement
    exists, because `outputs` is shorter than `inputs` or is null,
    it is deleted. If `outputs` is longer than `inputs`, excess
    bytes are ignored. Bytes that do not exist in `inputs` are left
    intact. This function returns a new string without modifying
    `text`.

  * Returns the translated string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.translate"));
    // Parse arguments.
    V_string text;
    V_string inputs;
    optV_string outputs;
    if(reader.I().v(text).v(inputs).o(outputs).F()) {
      Reference_root::S_temporary xref = { std_string_translate(::std::move(text), ::std::move(inputs),
                                                                ::std::move(outputs)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.explode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("explode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.explode(text, [delim], [limit])`

  * Breaks `text` down into segments, separated by `delim`. If
    `delim` is `null` or an empty string, every byte becomes a
    segment. If `limit` is set to a positive integer, there will be
    no more segments than this number; the vert last segment will
    contain all the remaining bytes of the `text`.

  * Returns an array containing the broken-down segments. If `text`
    is empty, an empty array is returned.

  * Throws an exception if `limit` is negative or zero.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.explode"));
    // Parse arguments.
    V_string text;
    optV_string delim;
    optV_integer limit;
    if(reader.I().v(text).o(delim).o(limit).F()) {
      Reference_root::S_temporary xref = { std_string_explode(::std::move(text), ::std::move(delim), limit) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.implode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("implode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.implode(segments, [delim])`

  * Concatenates elements of an array, `segments`, to create a new
    string. All segments shall be strings. If `delim` is
    specified, it is inserted between adjacent segments.

  * Returns a string containing all segments. If `segments` is
    empty, an empty string is returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.implode"));
    // Parse arguments.
    V_array segments;
    optV_string delim;
    if(reader.I().v(segments).o(delim).F()) {
      Reference_root::S_temporary xref = { std_string_implode(::std::move(segments), ::std::move(delim)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.hex_encode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("hex_encode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.hex_encode(data, [lowercase], [delim])`

  * Encodes all bytes in `data` as 2-digit hexadecimal numbers and
    concatenates them. If `lowercase` is set to `true`, hexadecimal
    digits above `9` are encoded as `abcdef`; otherwise they are
    encoded as `ABCDEF`. If `delim` is specified, it is inserted
    between adjacent bytes.

  * Returns the encoded string. If `data` is empty, an empty
    string is returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.hex_encode"));
    // Parse arguments.
    V_string data;
    optV_boolean lowercase;
    optV_string delim;
    if(reader.I().v(data).o(lowercase).o(delim).F()) {
      Reference_root::S_temporary xref = { std_string_hex_encode(::std::move(data), lowercase,
                                                                 ::std::move(delim)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.hex_decode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("hex_decode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.hex_decode(text)`

  * Decodes all hexadecimal digits from `text` and converts them to
    bytes. Whitespaces can be used to delimit bytes; they shall not
    occur between digits in the same byte. Consequently, the total
    number of non-whitespace characters must be a multiple of two.
    Invalid characters cause decode errors.

  * Returns a string containing decoded bytes. If `text` is empty
    or consists of only whitespaces, an empty string is returned.

  * Throws an exception if the string is invalid.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.hex_decode"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_hex_decode(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.base32_encode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("base32_encode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.base32_encode(data, [lowercase])`

  * Encodes all bytes in `data` according to the base32 encoding
    specified by IETF RFC 4648. If `lowercase` is set to `true`,
    lowercase letters are used to represent values through `0` to
    `25`; otherwise, uppercase letters are used. The length of
    encoded data is always a multiple of 8; padding characters are
    mandatory.

  * Returns the encoded string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.base32_encode"));
    // Parse arguments.
    V_string data;
    optV_boolean lowercase;
    if(reader.I().v(data).o(lowercase).F()) {
      Reference_root::S_temporary xref = { std_string_base32_encode(::std::move(data), lowercase) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.base32_decode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("base32_decode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.base32_decode(text)`

  * Decodes data encoded in base32, as specified by IETF RFC 4648.
    Whitespaces can be used to delimit encoding units; they shall
    not occur between characters in the same unit. Consequently,
    the number of non-whitespace characters must be a multiple of
    eight. Invalid characters cause decode errors.

  * Returns a string containing decoded bytes. If `text` is empty
    or consists of only whitespaces, an empty string is returned.

  * Throws an exception if the string is invalid.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.base32_decode"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_base32_decode(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.base64_encode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("base64_encode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.base64_encode(data)`

  * Encodes all bytes in `data` according to the base64 encoding
    specified by IETF RFC 4648. The length of encoded data is
    always a multiple of 4; padding characters are mandatory.

  * Returns the encoded string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.base64_encode"));
    // Parse arguments.
    V_string data;
    if(reader.I().v(data).F()) {
      Reference_root::S_temporary xref = { std_string_base64_encode(::std::move(data)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.base64_decode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("base64_decode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.base64_decode(text)`

  * Decodes data encoded in base64, as specified by IETF RFC 4648.
    Whitespaces can be used to delimit encoding units; they shall
    not occur between characters in the same unit. Consequently,
    the number of non-whitespace characters must be a multiple of
    four. Invalid characters cause decode errors.

  * Returns a string containing decoded bytes. If `text` is empty
    or consists of only whitespaces, an empty string is returned.

  * Throws an exception if the string is invalid.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.base64_decode"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_base64_decode(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.url_encode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("url_encode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.url_encode(data, [lowercase])`

  * Encodes bytes in `data` according to IETF RFC 3986. Every byte
    that is not a letter, digit, `-`, `.`, `_` or `~` is encoded as
    a `%` followed by two hexadecimal digits. If `lowercase` is set
    to `true`, lowercase letters are used to represent values
    through `10` to `15`; otherwise, uppercase letters are used.

  * Returns the encoded string. If `data` is empty, an empty
    string is returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.url_encode"));
    // Parse arguments.
    V_string data;
    optV_boolean lowercase;
    if(reader.I().v(data).o(lowercase).F()) {
      Reference_root::S_temporary xref = { std_string_url_encode(::std::move(data), lowercase) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.url_decode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("url_decode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.url_decode(text)`

  * Decodes percent-encode sequences from `text` and converts them
    to bytes according to IETF RFC 3986. For convenience reasons,
    both reserved and unreserved characters are copied verbatim.
    Characters that are neither reserved nor unreserved (such as
    ASCII control characters or non-ASCII characters) cause decode
    errors.

  * Returns a string containing decoded bytes.

  * Throws an exception if the string contains invalid characters.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.url_decode"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_url_decode(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.url_encode_query()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("url_encode_query"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.url_encode_query(data, [lowercase])`

  * Encodes bytes in `data` according to IETF RFC 3986. This
    function behaves like `url_encode()`, except that characters
    that are allowed unencoded in query strings are not encoded,
    and spaces are encoded as `+` instead of the long form `%20`.

  * Returns the encoded string. If `data` is empty, an empty
    string is returned.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.url_encode"));
    // Parse arguments.
    V_string data;
    optV_boolean lowercase;
    if(reader.I().v(data).o(lowercase).F()) {
      Reference_root::S_temporary xref = { std_string_url_encode_query(::std::move(data), lowercase) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.url_decode_query()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("url_decode_query"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.url_decode_query(text)`

  * Decodes percent-encode sequences from `text` and converts them
    to bytes according to IETF RFC 3986. This function behaves like
    `url_decode()`, except that `+` is decoded as a space.

  * Returns a string containing decoded bytes.

  * Throws an exception if the string contains invalid characters.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.url_decode"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_url_decode_query(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.utf8_validate()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utf8_validate"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.utf8_validate(text)`

  * Checks whether `text` is a valid UTF-8 string.

  * Returns `true` if `text` is valid, or `false` otherwise.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.utf8_validate"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_utf8_validate(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.utf8_encode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utf8_encode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.utf8_encode(code_points, [permissive])`

  * Encodes code points from `code_points` into an UTF-8 string.
    `code_points` can be either an integer or an array of
    integers. When an invalid code point is encountered, if
    `permissive` is set to `true`, it is replaced with the
    replacement character `"\uFFFD"` and consequently encoded as
    `"\xEF\xBF\xBD"`; otherwise this function fails.

  * Returns the encoded string.

  * Throws an exception on failure.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.utf8_encode"));
    // Parse arguments.
    V_integer code_point;
    optV_boolean permissive;
    if(reader.I().v(code_point).o(permissive).F()) {
      Reference_root::S_temporary xref = { std_string_utf8_encode(::std::move(code_point), permissive) };
      return self = ::std::move(xref);
    }
    V_array code_points;
    if(reader.I().v(code_points).o(permissive).F()) {
      Reference_root::S_temporary xref = { std_string_utf8_encode(::std::move(code_points), permissive) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.utf8_decode()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utf8_decode"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.utf8_decode(text, [permissive])`

  * Decodes `text`, which is expected to be a string containing
    UTF-8 code units, into an array of code points, represented as
    integers. When an invalid code sequence is encountered, if
    `permissive` is set to `true`, all code units of it are
    re-interpreted as isolated bytes according to ISO/IEC 8859-1;
    otherwise this function fails.

  * Returns an array containing decoded code points.

  * Throws an exception on failure.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.utf8_decode"));
    // Parse arguments.
    V_string text;
    optV_boolean permissive;
    if(reader.I().v(text).o(permissive).F()) {
      Reference_root::S_temporary xref = { std_string_utf8_decode(::std::move(text), permissive) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.pack_8()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_8"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.pack_8(values)`

  * Packs a series of 8-bit integers into a string. `values` can be
    either an integer or an array of integers, all of which are
    truncated to 8 bits then copied into a string.

  * Returns the packed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.pack_8"));
    // Parse arguments.
    V_integer value;
    if(reader.I().v(value).F()) {
      Reference_root::S_temporary xref = { std_string_pack_8(::std::move(value)) };
      return self = ::std::move(xref);
    }
    V_array values;
    if(reader.I().v(values).F()) {
      Reference_root::S_temporary xref = { std_string_pack_8(::std::move(values)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.unpack_8()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_8"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.unpack_8(text)`

  * Unpacks 8-bit integers from a string. The contents of `text`
    are re-interpreted as contiguous signed 8-bit integers, all of
    which are sign-extended to 64 bits then copied into an array.

  * Returns an array containing unpacked integers.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.unpack_8"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_unpack_8(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.pack_16be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_16be"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.pack_16be(values)`

  * Packs a series of 16-bit integers into a string. `values` can
    be either an integer or an array of `integers`, all of which
    are truncated to 16 bits then copied into a string in the
    big-endian byte order.

  * Returns the packed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.pack_16be"));
    // Parse arguments.
    V_integer value;
    if(reader.I().v(value).F()) {
      Reference_root::S_temporary xref = { std_string_pack_16be(::std::move(value)) };
      return self = ::std::move(xref);
    }
    V_array values;
    if(reader.I().v(values).F()) {
      Reference_root::S_temporary xref = { std_string_pack_16be(::std::move(values)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.unpack_16be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_16be"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.unpack_16be(text)`

  * Unpacks 16-bit integers from a string. The contents of `text`
    are re-interpreted as contiguous signed 16-bit integers in the
    big-endian byte order, all of which are sign-extended to 64
    bits then copied into an array.

  * Returns an array containing unpacked integers.

  * Throws an exception if the length of `text` is not a multiple
    of 2.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.unpack_16be"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_unpack_16be(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.pack_16le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_16le"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.pack_16le(values)`

  * Packs a series of 16-bit integers into a string. `values` can
    be either an integer or an array of `integers`, all of which
    are truncated to 16 bits then copied into a string in the
    little-endian byte order.

  * Returns the packed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.pack_16le"));
    // Parse arguments.
    V_integer value;
    if(reader.I().v(value).F()) {
      Reference_root::S_temporary xref = { std_string_pack_16le(::std::move(value)) };
      return self = ::std::move(xref);
    }
    V_array values;
    if(reader.I().v(values).F()) {
      Reference_root::S_temporary xref = { std_string_pack_16le(::std::move(values)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.unpack_16le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_16le"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.unpack_16le(text)`

  * Unpacks 16-bit integers from a string. The contents of `text`
    are re-interpreted as contiguous signed 16-bit integers in the
    little-endian byte order, all of which are sign-extended to 64
    bits then copied into an array.

  * Returns an array containing unpacked integers.

  * Throws an exception if the length of `text` is not a multiple
    of 2.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.unpack_16le"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_unpack_16le(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.pack_32be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_32be"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.pack_32be(values)`

  * Packs a series of 32-bit integers into a string. `values` can
    be either an integer or an array of `integers`, all of which
    are truncated to 32 bits then copied into a string in the
    big-endian byte order.

  * Returns the packed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.pack_32be"));
    // Parse arguments.
    V_integer value;
    if(reader.I().v(value).F()) {
      Reference_root::S_temporary xref = { std_string_pack_32be(::std::move(value)) };
      return self = ::std::move(xref);
    }
    V_array values;
    if(reader.I().v(values).F()) {
      Reference_root::S_temporary xref = { std_string_pack_32be(::std::move(values)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.unpack_32be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_32be"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.unpack_32be(text)`

  * Unpacks 32-bit integers from a string. The contents of `text`
    are re-interpreted as contiguous signed 32-bit integers in the
    big-endian byte order, all of which are sign-extended to 64
    bits then copied into an array.

  * Returns an array containing unpacked integers.

  * Throws an exception if the length of `text` is not a multiple
    of 4.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.unpack_32be"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_unpack_32be(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.pack_32le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_32le"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.pack_32le(values)`

  * Packs a series of 32-bit integers into a string. `values` can
    be either an integer or an array of `integers`, all of which
    are truncated to 32 bits then copied into a string in the
    little-endian byte order.

  * Returns the packed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.pack_32le"));
    // Parse arguments.
    V_integer value;
    if(reader.I().v(value).F()) {
      Reference_root::S_temporary xref = { std_string_pack_32le(::std::move(value)) };
      return self = ::std::move(xref);
    }
    V_array values;
    if(reader.I().v(values).F()) {
      Reference_root::S_temporary xref = { std_string_pack_32le(::std::move(values)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.unpack_32le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_32le"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.unpack_32le(text)`

  * Unpacks 32-bit integers from a string. The contents of `text`
    are re-interpreted as contiguous signed 32-bit integers in the
    little-endian byte order, all of which are sign-extended to 64
    bits then copied into an array.

  * Returns an array containing unpacked integers.

  * Throws an exception if the length of `text` is not a multiple
    of 4.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.unpack_32le"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_unpack_32le(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.pack_64be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_64be"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.pack_64be(values)`

  * Packs a series of 64-bit integers into a string. `values` can
    be either an integer or an array of `integers`, all of which
    are copied into a string in the big-endian byte order.

  * Returns the packed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.pack_64be"));
    // Parse arguments.
    V_integer value;
    if(reader.I().v(value).F()) {
      Reference_root::S_temporary xref = { std_string_pack_64be(::std::move(value)) };
      return self = ::std::move(xref);
    }
    V_array values;
    if(reader.I().v(values).F()) {
      Reference_root::S_temporary xref = { std_string_pack_64be(::std::move(values)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.unpack_64be()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_64be"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.unpack_64be(text)`

  * Unpacks 64-bit integers from a string. The contents of `text`
    are re-interpreted as contiguous signed 64-bit integers in the
    big-endian byte order, all of which are copied into an array.

  * Returns an array containing unpacked integers.

  * Throws an exception if the length of `text` is not a multiple
    of 8.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.unpack_64be"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_unpack_64be(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.pack_64le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("pack_64le"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.pack_64le(values)`

  * Packs a series of 64-bit integers into a string. `values` can
    be either an integer or an array of `integers`, all of which
    are copied into a string in the little-endian byte order.

  * Returns the packed string.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.pack_64le"));
    // Parse arguments.
    V_integer value;
    if(reader.I().v(value).F()) {
      Reference_root::S_temporary xref = { std_string_pack_64le(::std::move(value)) };
      return self = ::std::move(xref);
    }
    V_array values;
    if(reader.I().v(values).F()) {
      Reference_root::S_temporary xref = { std_string_pack_64le(::std::move(values)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.unpack_64le()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("unpack_64le"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.unpack_64le(text)`

  * Unpacks 64-bit integers from a string. The contents of `text`
    are re-interpreted as contiguous signed 64-bit integers in the
    little-endian byte order, all of which are copied into an
    array.

  * Returns an array containing unpacked integers.

  * Throws an exception if the length of `text` is not a multiple
    of 8.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.unpack_64le"));
    // Parse arguments.
    V_string text;
    if(reader.I().v(text).F()) {
      Reference_root::S_temporary xref = { std_string_unpack_64le(::std::move(text)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.format()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("format"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.format(templ, ...)`

  * Compose a string according to the template string `templ`, as
    follows:

    * A sequence of `$$` is replaced with a literal `$`.
    * A sequence of `${NNN}`, where `NNN` is at most three decimal
      numerals, is replaced with the NNN-th argument. If `NNN` is
      zero, it is replaced with `templ` itself.
    * A sequence of `$N`, where `N` is a single decimal numeral,
      behaves the same as `${N}`.
    * All other characters are copied verbatim.

  * Returns the composed string.

  * Throws an exception if `templ` contains invalid placeholder
    sequences, or when a placeholder sequence has no corresponding
    argument.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.format"));
    // Parse arguments.
    V_string templ;
    cow_vector<Value> values;
    if(reader.I().v(templ).F(values)) {
      Reference_root::S_temporary xref = { std_string_format(::std::move(templ), ::std::move(values)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.regex_find()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("regex_find"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.regex_find(text, pattern)`

  * Searches `text` for the first occurrence of the regular
    expression `pattern`.

  * Returns an array of two integers, the first of which
    specifies the subscript of the matching sequence and the second
    of which specifies its length. If `pattern` is not found, this
    function returns `null`.

  * Throws an exception if `pattern` is not a valid regular
    expression.

`std.string.regex_find(text, from, pattern)`

  * Searches `text` for the first occurrence of the regular
    expression `pattern`. The search operation is performed on the
    same subrange that would be returned by `slice(text, from)`.

  * Returns an array of two integers, the first of which
    specifies the subscript of the matching sequence and the second
    of which specifies its length. If `pattern` is not found, this
    function returns `null`.

  * Throws an exception if `pattern` is not a valid regular
    expression.

`std.string.regex_find(text, from, [length], pattern)`

  * Searches `text` for the first occurrence of the regular
    expression `pattern`. The search operation is performed on the
    same subrange that would be returned by
    `slice(text, from, length)`.

  * Returns an array of two integers, the first of which
    specifies the subscript of the matching sequence and the second
    of which specifies its length. If `pattern` is not found, this
    function returns `null`.

  * Throws an exception if `pattern` is not a valid regular
    expression.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.regex_find"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_string pattern;
    if(reader.I().v(text).S(state).v(pattern).F()) {
      auto kpair = std_string_regex_find(::std::move(text), 0, nullopt, ::std::move(pattern));
      if(!kpair)
        return self = Reference_root::S_temporary();
      // The binding function returns a `pair`, but we would like to return an array so convert it.
      Reference_root::S_temporary xref = { { kpair->first, kpair->second } };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(pattern).F()) {
      auto kpair = std_string_regex_find(::std::move(text), from, nullopt, ::std::move(pattern));
      if(!kpair)
        return self = Reference_root::S_temporary();
      // The binding function returns a `pair`, but we would like to return an array so convert it.
      Reference_root::S_temporary xref = { { kpair->first, kpair->second } };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(pattern).F()) {
      auto kpair = std_string_regex_find(::std::move(text), from, length, ::std::move(pattern));
      if(!kpair)
        return self = Reference_root::S_temporary();
      // The binding function returns a `pair`, but we would like to return an array so convert it.
      Reference_root::S_temporary xref = { { kpair->first, kpair->second } };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.regex_match()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("regex_match"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.regex_match(text, pattern)`

  * Checks whether the regular expression `patterm` matches the
    entire sequence `text`.

  * Returns an array of optional strings. The first element
    contains a copy of `text`. All the other elements hold
    substrings that match positional capturing groups. If a group
    matches nothing, the corresponding element is `null`. The total
    number of elements equals the number of capturing groups plus
    one. If `text` does not match `pattern`, `null` is returned.

  * Throws an exception if `pattern` is not a valid regular
    expression.

`std.string.regex_match(text, from, pattern)`

  * Checks whether the regular expression `patterm` matches the
    subrange that would be returned by `slice(text, from)`.

  * Returns an array of optional strings. The first element
    contains a copy of `text`. All the other elements hold
    substrings that match positional capturing groups. If a group
    matches nothing, the corresponding element is `null`. The total
    number of elements equals the number of capturing groups plus
    one. If `text` does not match `pattern`, `null` is returned.

  * Throws an exception if `pattern` is not a valid regular
    expression.

`std.string.regex_match(text, from, [length], pattern)`

  * Checks whether the regular expression `patterm` matches the
    subrange that would be returned by `slice(text, from, length)`.

  * Returns an array of optional strings. The first element
    contains a copy of `text`. All the other elements hold
    substrings that match positional capturing groups. If a group
    matches nothing, the corresponding element is `null`. The total
    number of elements equals the number of capturing groups plus
    one. If `text` does not match `pattern`, `null` is returned.

  * Throws an exception if `pattern` is not a valid regular
    expression.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.regex_match"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_string pattern;
    if(reader.I().v(text).S(state).v(pattern).F()) {
      Reference_root::S_temporary xref = { std_string_regex_match(::std::move(text), 0, nullopt,
                                                                  ::std::move(pattern)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(pattern).F()) {
      Reference_root::S_temporary xref = { std_string_regex_match(::std::move(text), from, nullopt,
                                                                  ::std::move(pattern)) };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(pattern).F()) {
      Reference_root::S_temporary xref = { std_string_regex_match(::std::move(text), from, length,
                                                                  ::std::move(pattern)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.string.regex_replace()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("regex_replace"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.string.regex_replace(text, pattern, replacement)`

  * Searches `text` and replaces all matches of the regular
    expression `pattern` with `replacement`. This function returns
    a new string without modifying `text`.

  * Returns the string with `pattern` replaced. If `text` does not
    contain `pattern`, it is returned intact.

  * Throws an exception if `pattern` is not a valid regular
    expression.

`std.string.regex_replace(text, from, pattern, replacement)`

  * Searches `text` and replaces all matches of the regular
    expression `pattern` with `replacement`. The search operation
    is performed on the same subrange that would be returned by
    `slice(text, from)`. This function returns a new string
    without modifying `text`.

  * Returns the string with `pattern` replaced. If `text` does not
    contain `pattern`, it is returned intact.

  * Throws an exception if either `pattern` or `replacement` is not
    a valid regular expression.

`std.string.regex_replace(text, from, [length], pattern, replacement)`

  * Searches `text` and replaces all matches of the regular
    expression `pattern` with `replacement`. The search operation
    is performed on the same subrange that would be returned by
    `slice(text, from, length)`. This function returns a new
    string without modifying `text`.

  * Returns the string with `pattern` replaced. If `text` does not
    contain `pattern`, it is returned intact.

  * Throws an exception if `pattern` is not a valid regular
    expression.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.string.regex_replace"));
    Argument_Reader::State state;
    // Parse arguments.
    V_string text;
    V_string pattern;
    V_string replacement;
    if(reader.I().v(text).S(state).v(pattern).v(replacement).F()) {
      Reference_root::S_temporary xref = { std_string_regex_replace(::std::move(text), 0, nullopt,
                                                          ::std::move(pattern), ::std::move(replacement)) };
      return self = ::std::move(xref);
    }
    V_integer from;
    if(reader.L(state).v(from).S(state).v(pattern).v(replacement).F()) {
      Reference_root::S_temporary xref = { std_string_regex_replace(::std::move(text), from, nullopt,
                                                           ::std::move(pattern), ::std::move(replacement)) };
      return self = ::std::move(xref);
    }
    optV_integer length;
    if(reader.L(state).o(length).v(pattern).v(replacement).F()) {
      Reference_root::S_temporary xref = { std_string_regex_replace(::std::move(text), from, length,
                                                           ::std::move(pattern), ::std::move(replacement)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));
  }

}  // namespace asteria
