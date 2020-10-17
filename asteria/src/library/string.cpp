// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "string.hpp"
#include "../runtime/argument_reader.hpp"
#include "../util.hpp"
#include "../../rocket/ascii_case.hpp"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <endian.h>

namespace asteria {
namespace {

pair<V_string::const_iterator, V_string::const_iterator>
do_slice(const V_string& text, V_string::const_iterator tbegin, const Opt_integer& length)
  {
    if(!length || (*length >= text.end() - tbegin))
      return ::std::make_pair(tbegin, text.end());

    if(*length <= 0)
      return ::std::make_pair(tbegin, tbegin);

    // Don't go past the end.
    return ::std::make_pair(tbegin, tbegin + static_cast<ptrdiff_t>(*length));
  }

pair<V_string::const_iterator, V_string::const_iterator>
do_slice(const V_string& text, const V_integer& from, const Opt_integer& length)
  {
    auto slen = static_cast<int64_t>(text.size());
    if(from >= 0) {
      // Behave like `::std::string::substr()` except that no exception is thrown when `from`
      // is greater than `text.size()`.
      if(from >= slen)
        return ::std::make_pair(text.end(), text.end());

      // Return a subrange from `begin() + from`.
      return do_slice(text, text.begin() + static_cast<ptrdiff_t>(from), length);
    }

    // Wrap `from` from the end. Notice that `from + slen` will not overflow when `from` is
    // negative and `slen` is not.
    auto rfrom = from + slen;
    if(rfrom >= 0)
      return do_slice(text, text.begin() + static_cast<ptrdiff_t>(rfrom), length);

    // Get a subrange from the beginning of `text`, if the wrapped index is before the first
    // byte.
    if(!length)
      return ::std::make_pair(text.begin(), text.end());

    if(*length <= 0)
      return ::std::make_pair(text.begin(), text.begin());

    // Get a subrange excluding the part before the beginning.
    // Notice that `rfrom + *length` will not overflow when `rfrom` is negative and `*length`
    // is not.
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
opt<IterT>
do_find_of_opt(IterT begin, IterT end, const V_string& set, bool match)
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

V_string
do_get_reject(const Opt_string& reject)
  {
    if(!reject)
      return ::rocket::sref(" \t");
    return *reject;
  }

V_string
do_get_padding(const Opt_string& padding)
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
V_string
do_url_decode(const V_string& text)
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

ROCKET_CONST_FUNCTION inline
int64_t
bswap_be(int64_t value)
  noexcept
  { return static_cast<int64_t>(be64toh(static_cast<uint64_t>(value)));  }

ROCKET_CONST_FUNCTION inline
int32_t
bswap_be(int32_t value)
  noexcept
  { return static_cast<int32_t>(be32toh(static_cast<uint32_t>(value)));  }

ROCKET_CONST_FUNCTION inline
int16_t
bswap_be(int16_t value)
  noexcept
  { return static_cast<int16_t>(be16toh(static_cast<uint16_t>(value)));  }

ROCKET_CONST_FUNCTION inline
int8_t
bswap_be(int8_t value)
  noexcept
  { return value;  }

ROCKET_CONST_FUNCTION inline
int64_t
bswap_le(int64_t value)
  noexcept
  { return static_cast<int64_t>(le64toh(static_cast<uint64_t>(value)));  }

ROCKET_CONST_FUNCTION inline
int32_t
bswap_le(int32_t value)
  noexcept
  { return static_cast<int32_t>(le32toh(static_cast<uint32_t>(value)));  }

ROCKET_CONST_FUNCTION inline
int16_t
bswap_le(int16_t value)
  noexcept
  { return static_cast<int16_t>(le16toh(static_cast<uint16_t>(value)));  }

ROCKET_CONST_FUNCTION inline
int8_t
bswap_le(int8_t value)
  noexcept
  { return value;  }

template<typename WordT>
V_string
do_pack_be(const V_integer& value)
  {
    WordT word = bswap_be(static_cast<WordT>(value));
    V_string text(reinterpret_cast<char*>(&word), sizeof(WordT));
    return text;
  }

template<typename WordT>
V_string
do_pack_be(const V_array& values)
  {
    V_string text;
    text.reserve(values.size() * sizeof(WordT));
    for(const auto& value : values) {
      WordT word = bswap_be(static_cast<WordT>(value.as_integer()));
      text.append(reinterpret_cast<char*>(&word), sizeof(WordT));
    }
    return text;
  }

template<typename WordT>
V_string
do_pack_le(const V_integer& value)
  {
    WordT word = bswap_le(static_cast<WordT>(value));
    V_string text(reinterpret_cast<char*>(&word), sizeof(WordT));
    return text;
  }

template<typename WordT>
V_string
do_pack_le(const V_array& values)
  {
    V_string text;
    text.reserve(values.size() * sizeof(WordT));
    for(const auto& value : values) {
      WordT word = bswap_le(static_cast<WordT>(value.as_integer()));
      text.append(reinterpret_cast<char*>(&word), sizeof(WordT));
    }
    return text;
  }

template<typename WordT>
V_array
do_unpack_be(const V_string& text)
  {
    size_t nwords = text.size() / sizeof(WordT);
    if(nwords * sizeof(WordT) != text.size())
      ASTERIA_THROW("String length `$1` not divisible by `$2`", text.size(), sizeof(WordT));

    V_array values;
    values.reserve(nwords);
    for(size_t k = 0;  k != nwords;  ++k) {
      WordT word;
      ::std::memcpy(&word, text.data() + k * sizeof(WordT), sizeof(WordT));
      values.emplace_back(V_integer(bswap_be(word)));
    }
    return values;
  }

template<typename WordT>
V_array
do_unpack_le(const V_string& text)
  {
    size_t nwords = text.size() / sizeof(WordT);
    if(nwords * sizeof(WordT) != text.size())
      ASTERIA_THROW("String length `$1` not divisible by `$2`", text.size(), sizeof(WordT));

    V_array values	;
    values.reserve(nwords);
    for(size_t k = 0;  k != nwords;  ++k) {
      WordT word;
      ::std::memcpy(&word, text.data() + k * sizeof(WordT), sizeof(WordT));
      values.emplace_back(V_integer(bswap_le(word)));
    }
    return values;
  }

class PCRE2_Error
  {
  private:
    array<char, 256> m_buf;

  public:
    explicit
    PCRE2_Error(int err)
      noexcept
      { ::pcre2_get_error_message(err, reinterpret_cast<uint8_t*>(this->m_buf.mut_data()),
                                  this->m_buf.size());  }

  public:
    const char*
    c_str()
      const noexcept
      { return this->m_buf.data();  }
  };

inline
tinyfmt&
operator<<(tinyfmt& fmt, const PCRE2_Error& err)
  { return fmt << err.c_str();  }

class PCRE2_pcre
  {
  private:
    uptr<::pcre2_code, void (&)(::pcre2_code*)> m_code;
    uptr<::pcre2_match_data, void (&)(::pcre2_match_data*)> m_match;

  public:
    explicit
    PCRE2_pcre(const V_string& pattern, uint32_t opts = 0)
      : m_code(::pcre2_code_free),
        m_match(::pcre2_match_data_free)
      {
        int err;
        size_t off;
        if(!this->m_code.reset(::pcre2_compile(reinterpret_cast<const uint8_t*>(pattern.data()),
                               pattern.size(), opts | PCRE2_NEVER_UTF | PCRE2_NEVER_UCP,
                               &err, &off, nullptr)))
          ASTERIA_THROW("Invalid regular expression: $1\n"
                        "[`pcre2_compile()` failed at offset `$3`: $2]",
                        pattern, PCRE2_Error(err), off);

        if(!this->m_match.reset(::pcre2_match_data_create_from_pattern(this->m_code, nullptr)))
          ASTERIA_THROW("Could not allocate `match_data` structure: $1\n"
                        "[`pcre2_match_data_create_from_pattern()` failed]",
                        pattern);
      }

  public:
    ::pcre2_code*
    code()
      noexcept
      { return this->m_code;  }

    ::pcre2_match_data*
    match()
      noexcept
      { return this->m_match;  }
  };

struct PCRE2_Name
  {
    uint16_t index_be;
    char name[];
  };

}  // namespace

V_string
std_string_slice(V_string text, V_integer from, Opt_integer length)
  {
    // Use reference counting as our advantage.
    V_string res = text;
    auto range = do_slice(res, from, length);
    if(range.second - range.first != res.ssize())
      res.assign(range.first, range.second);
    return res;
  }

V_string
std_string_replace_slice(V_string text, V_integer from, Opt_integer length,
                         V_string replacement, Opt_integer rfrom, Opt_integer rlength)
  {
    V_string res = text;
    auto range = do_slice(res, from, length);
    auto rep_range = do_slice(replacement, rfrom.value_or(0), rlength);

    // Replace the subrange.
    res.replace(range.first, range.second, rep_range.first, rep_range.second);
    return res;
  }

V_integer
std_string_compare(V_string text1, V_string text2, Opt_integer length)
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

Opt_integer
std_string_find(V_string text, V_integer from, Opt_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
    if(!qit)
      return nullopt;
    return *qit - text.begin();
  }

Opt_integer
std_string_rfind(V_string text, V_integer from, Opt_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second),
                           ::std::make_reverse_iterator(range.first),
                           pattern.rbegin(), pattern.rend());
    if(!qit)
      return nullopt;
    return text.rend() - *qit - pattern.ssize();
  }

V_string
std_string_find_and_replace(V_string text, V_integer from, Opt_integer length, V_string pattern,
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

Opt_integer
std_string_find_any_of(V_string text, V_integer from, Opt_integer length, V_string accept)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(range.first, range.second, accept, true);
    if(!qit)
      return nullopt;
    return *qit - text.begin();
  }

Opt_integer
std_string_find_not_of(V_string text, V_integer from, Opt_integer length, V_string reject)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(range.first, range.second, reject, false);
    if(!qit)
      return nullopt;
    return *qit - text.begin();
  }

Opt_integer
std_string_rfind_any_of(V_string text, V_integer from, Opt_integer length, V_string accept)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(::std::make_reverse_iterator(range.second),
                              ::std::make_reverse_iterator(range.first), accept, true);
    if(!qit)
      return nullopt;
    return text.rend() - *qit - 1;
  }

Opt_integer
std_string_rfind_not_of(V_string text, V_integer from, Opt_integer length, V_string reject)
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
std_string_trim(V_string text, Opt_string reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0)
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Get the index of the first byte to keep.
    size_t bpos = text.find_first_not_of(rchars);
    if(bpos == V_string::npos)
      // There is no byte to keep. Return an empty string.
      return { };

    // Get the index of the last byte to keep.
    size_t epos = text.find_last_not_of(rchars) + 1;
    if((bpos == 0) && (epos == text.size()))
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Return the remaining part of `text`.
    return text.substr(bpos, epos - bpos);
  }

V_string
std_string_triml(V_string text, Opt_string reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0)
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Get the index of the first byte to keep.
    size_t bpos = text.find_first_not_of(rchars);
    if(bpos == V_string::npos)
      // There is no byte to keep. Return an empty string.
      return { };

    if(bpos == 0)
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Return the remaining part of `text`.
    return text.substr(bpos);
  }

V_string
std_string_trimr(V_string text, Opt_string reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0)
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Get the index of the last byte to keep.
    size_t epos = text.find_last_not_of(rchars) + 1;
    if(epos == 0)
      // There is no byte to keep. Return an empty string.
      return { };

    if(epos == text.size())
      // There is no byte to strip. Make use of reference counting.
      return text;

    // Return the remaining part of `text`.
    return text.substr(0, epos);
  }

V_string
std_string_padl(V_string text, V_integer length, Opt_string padding)
  {
    V_string res = text;
    auto rpadding = do_get_padding(padding);
    if(length <= 0)
      return res;

    // Fill `rpadding` at the front.
    res.reserve(static_cast<size_t>(length));
    while(res.size() + rpadding.length() <= static_cast<uint64_t>(length))
      res.insert(res.end() - text.ssize(), rpadding);
    return res;
  }

V_string
std_string_padr(V_string text, V_integer length, Opt_string padding)
  {
    V_string res = text;
    auto rpadding = do_get_padding(padding);
    if(length <= 0)
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
      char t = ::rocket::ascii_to_upper(c);
      if(c == t)
        continue;

      // Fork the string as needed.
      if(ROCKET_UNEXPECT(!wptr))
        wptr = res.mut_data();
      wptr[i] = t;
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
      char t = ::rocket::ascii_to_lower(c);
      if(c == t)
        continue;

      // Fork the string as needed.
      if(ROCKET_UNEXPECT(!wptr))
        wptr = res.mut_data();

      wptr[i] = t;
    }
    return res;
  }

V_string
std_string_translate(V_string text, V_string inputs, Opt_string outputs)
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
std_string_explode(V_string text, Opt_string delim, Opt_integer limit)
  {
    uint64_t rlimit = UINT64_MAX;
    if(limit) {
      if(*limit <= 0)
        ASTERIA_THROW("Max number of segments must be positive (limit `$1`)", *limit);
      rlimit = static_cast<uint64_t>(*limit);
    }

    // Return an empty array if there is no segment.
    V_array segments;
    if(text.empty())
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
std_string_implode(V_array segments, Opt_string delim)
  {
    V_string text;

    // Return an empty string if there is no segment.
    auto nsegs = segments.size();
    if(nsegs == 0)
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
std_string_hex_encode(V_string data, Opt_boolean lowercase, Opt_string delim)
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
std_string_base32_encode(V_string data, Opt_boolean lowercase)
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
std_string_url_encode(V_string data, Opt_boolean lowercase)
  {
    return do_url_encode<0>(data, lowercase.value_or(false));
  }

V_string
std_string_url_decode(V_string text)
  {
    return do_url_decode<0>(text);
  }

V_string
std_string_url_encode_query(V_string data, Opt_boolean lowercase)
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
std_string_utf8_encode(V_integer code_point, Opt_boolean permissive)
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
std_string_utf8_encode(V_array code_points, Opt_boolean permissive)
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
std_string_utf8_decode(V_string text, Opt_boolean permissive)
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
    return do_pack_be<int8_t>(value);
  }

V_string
std_string_pack_8(V_array values)
  {
    return do_pack_be<int8_t>(values);
  }

V_array
std_string_unpack_8(V_string text)
  {
    return do_unpack_be<int8_t>(text);
  }

V_string
std_string_pack_16be(V_integer value)
  {
    return do_pack_be<int16_t>(value);
  }

V_string
std_string_pack_16be(V_array values)
  {
    return do_pack_be<int16_t>(values);
  }

V_array
std_string_unpack_16be(V_string text)
  {
    return do_unpack_be<int16_t>(text);
  }

V_string
std_string_pack_16le(V_integer value)
  {
    return do_pack_le<int16_t>(value);
  }

V_string
std_string_pack_16le(V_array values)
  {
    return do_pack_le<int16_t>(values);
  }

V_array
std_string_unpack_16le(V_string text)
  {
    return do_unpack_le<int16_t>(text);
  }

V_string
std_string_pack_32be(V_integer value)
  {
    return do_pack_be<int32_t>(value);
  }

V_string
std_string_pack_32be(V_array values)
  {
    return do_pack_be<int32_t>(values);
  }

V_array
std_string_unpack_32be(V_string text)
  {
    return do_unpack_be<int32_t>(text);
  }

V_string
std_string_pack_32le(V_integer value)
  {
    return do_pack_le<int32_t>(value);
  }

V_string
std_string_pack_32le(V_array values)
  {
    return do_pack_le<int32_t>(values);
  }

V_array
std_string_unpack_32le(V_string text)
  {
    return do_unpack_le<int32_t>(text);
  }

V_string
std_string_pack_64be(V_integer value)
  {
    return do_pack_be<int64_t>(value);
  }

V_string
std_string_pack_64be(V_array values)
  {
    return do_pack_be<int64_t>(values);
  }

V_array
std_string_unpack_64be(V_string text)
  {
    return do_unpack_be<int64_t>(text);
  }

V_string
std_string_pack_64le(V_integer value)
  {
    return do_pack_le<int64_t>(value);
  }

V_string
std_string_pack_64le(V_array values)
  {
    return do_pack_le<int64_t>(values);
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
std_string_pcre_find(V_string text, V_integer from, Opt_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);

    // Get the real start and length.
    auto sub_off = static_cast<size_t>(range.first - text.begin());
    auto sub_ptr = reinterpret_cast<const uint8_t*>(text.data()) + sub_off;
    auto sub_len = static_cast<size_t>(range.second - range.first);

    // Try matching using default options.
    PCRE2_pcre pcre(pattern);
    int err = ::pcre2_match(pcre.code(), sub_ptr, sub_len, 0, 0, pcre.match(), nullptr);
    if(err < 0) {
      if(err == PCRE2_ERROR_NOMATCH)
        return nullopt;

      ASTERIA_THROW("Regular expression match failure: $1\n"
                    "[`pcre2_match()` failed: $2]",
                    pattern, PCRE2_Error(err));
    }
    auto ovec = ::pcre2_get_ovector_pointer(pcre.match());

    // This is copied from PCRE2 manual:
    //   If a pattern uses the \K escape sequence within a positive assertion, the reported
    //   start of a successful match can be greater than the end of the match. For example,
    //   if the pattern (?=ab\K) is matched against "ab", the start and end offset values
    //   for the match are 2 and 0.
    return ::std::make_pair(static_cast<int64_t>(sub_off + ovec[0]),
                            static_cast<int64_t>(::std::max(ovec[0], ovec[1]) - ovec[0]));
  }

Opt_array
std_string_pcre_match(V_string text, V_integer from, Opt_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);

    // Get the real start and length.
    auto sub_off = static_cast<size_t>(range.first - text.begin());
    auto sub_ptr = reinterpret_cast<const uint8_t*>(text.data()) + sub_off;
    auto sub_len = static_cast<size_t>(range.second - range.first);

    // Try matching using default options.
    PCRE2_pcre pcre(pattern);
    int err = ::pcre2_match(pcre.code(), sub_ptr, sub_len, 0, 0, pcre.match(), nullptr);
    if(err < 0) {
      if(err == PCRE2_ERROR_NOMATCH)
        return nullopt;

      ASTERIA_THROW("Regular expression match failure: $1\n"
                    "[`pcre2_match()` failed: $2]",
                    pattern, PCRE2_Error(err));
    }
    auto ovec = ::pcre2_get_ovector_pointer(pcre.match());
    size_t npairs = ::pcre2_get_ovector_count(pcre.match());

    // Compose the match result array.
    // The first element should be the matched substring. All remaining elements are
    // positional capturing groups. If a group matched nothing, its corresponding element
    // is `null`.
    V_array matches(npairs);
    for(size_t k = 0;  k != npairs;  ++k) {
      // This is copied from PCRE2 manual:
      //   If a pattern uses the \K escape sequence within a positive assertion, the reported
      //   start of a successful match can be greater than the end of the match. For example,
      //   if the pattern (?=ab\K) is matched against "ab", the start and end offset values
      //   for the match are 2 and 0.
      auto opair = ovec + k * 2;
      if(opair[0] != PCRE2_UNSET)
        matches.mut(k) = cow_string(reinterpret_cast<const char*>(sub_ptr + opair[0]),
                                    ::std::max(opair[0], opair[1]) - opair[0]);
    }
    return ::std::move(matches);
  }

Opt_object
std_string_pcre_named_match(V_string text, V_integer from, Opt_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);

    // Get the real start and length.
    auto sub_off = static_cast<size_t>(range.first - text.begin());
    auto sub_ptr = reinterpret_cast<const uint8_t*>(text.data()) + sub_off;
    auto sub_len = static_cast<size_t>(range.second - range.first);

    // Try matching using default options.
    PCRE2_pcre pcre(pattern);
    int err = ::pcre2_match(pcre.code(), sub_ptr, sub_len, 0, 0, pcre.match(), nullptr);
    if(err < 0) {
      if(err == PCRE2_ERROR_NOMATCH)
        return nullopt;

      ASTERIA_THROW("Regular expression match failure: $1\n"
                    "[`pcre2_match()` failed: $2]",
                    pattern, PCRE2_Error(err));
    }
    auto ovec = ::pcre2_get_ovector_pointer(pcre.match());

    // Get named group information.
    const uint8_t* gptr;
    ::pcre2_pattern_info(pcre.code(), PCRE2_INFO_NAMETABLE, &gptr);

    uint32_t ngroups;
    ::pcre2_pattern_info(pcre.code(), PCRE2_INFO_NAMECOUNT, &ngroups);

    uint32_t gsize;
    ::pcre2_pattern_info(pcre.code(), PCRE2_INFO_NAMEENTRYSIZE, &gsize);

    // Compose the match result object.
    V_object matches;
    for(size_t k = 0;  k != ngroups;  ++k) {
      // Get the index and name of this group.
      auto gcur = reinterpret_cast<const PCRE2_Name*>(gptr + k * gsize);
      size_t gindex = be16toh(gcur->index_be);
      auto gmatch = matches.try_emplace(cow_string(gcur->name)).first;

      // This is copied from PCRE2 manual:
      //   If a pattern uses the \K escape sequence within a positive assertion, the reported
      //   start of a successful match can be greater than the end of the match. For example,
      //   if the pattern (?=ab\K) is matched against "ab", the start and end offset values
      //   for the match are 2 and 0.
      auto opair = ovec + gindex * 2;
      if(opair[0] != PCRE2_UNSET)
        gmatch->second = cow_string(reinterpret_cast<const char*>(sub_ptr + opair[0]),
                                    ::std::max(opair[0], opair[1]) - opair[0]);
    }
    return ::std::move(matches);
  }

V_string
std_string_pcre_replace(V_string text, V_integer from, Opt_integer length, V_string pattern,
                        V_string replacement)
  {
    auto range = do_slice(text, from, length);

    // Get the real start and length.
    auto sub_off = static_cast<size_t>(range.first - text.begin());
    auto sub_ptr = reinterpret_cast<const uint8_t*>(text.data()) + sub_off;
    auto sub_len = static_cast<size_t>(range.second - range.first);

    // Reserve resonable storage for the replaced string.
    size_t output_len = replacement.size() + text.size() + 15;
    V_string output_str;

    // Try matching using default options.
    PCRE2_pcre pcre(pattern);
    int err;
    do {
      output_str.assign(output_len, '*');

      err = ::pcre2_substitute(pcre.code(), sub_ptr, sub_len, 0,
                   PCRE2_SUBSTITUTE_EXTENDED | PCRE2_SUBSTITUTE_GLOBAL
                     | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH,
                   pcre.match(), nullptr,
                   reinterpret_cast<const uint8_t*>(replacement.data()), replacement.size(),
                   reinterpret_cast<uint8_t*>(output_str.mut_data()), &output_len);
    }
    while(err == PCRE2_ERROR_NOMEMORY);

    if(err < 0) {
      if(err == PCRE2_ERROR_NOMATCH)
        return ::std::move(text);

      ASTERIA_THROW("Regular expression substitution failure: $1\n"
                    "[`pcre2_substitute()` failed: $2]",
                    pattern, PCRE2_Error(err));
    }

    // Discard excess characters.
    ROCKET_ASSERT(output_len <= output_str.size());
    output_str.erase(output_len);

    // Concatenate it with unreplaced parts.
    output_str.insert(output_str.begin(), text.begin(), range.first);
    output_str.append(range.second, text.end());
    return output_str;
  }

void
create_bindings_string(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(::rocket::sref("slice"),
      ASTERIA_BINDING_BEGIN("std.string.slice", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;

        reader.start_overload();
        reader.required(text);      // text
        reader.required(from);      // from
        reader.optional(len);       // [length]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_slice, text, from, len);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("replace_slice"),
      ASTERIA_BINDING_BEGIN("std.string.replace_slice", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string rep;
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
                    std_string_replace_slice, text, from, nullopt, rep, rfrom, rlen);

        reader.load_state(0);       // text, from
        reader.optional(len);       // [length]
        reader.required(rep);       // replacement
        reader.optional(rfrom);     // rep_from
        reader.optional(rlen);      // rep_length
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_replace_slice, text, from, len, rep, rfrom, rlen);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("compare"),
      ASTERIA_BINDING_BEGIN("std.string.compare", self, global, reader) {
        V_string text1;
        V_string text2;
        Opt_integer len;

        reader.start_overload();
        reader.required(text1);     // text1
        reader.required(text2);     // text2
        reader.optional(len);       // [length]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_compare, text1, text2, len);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("starts_with"),
      ASTERIA_BINDING_BEGIN("std.string.starts_with", self, global, reader) {
        V_string text;
        V_string prfx;

        reader.start_overload();
        reader.required(text);     // text
        reader.required(prfx);     // prefix
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_starts_with, text, prfx);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("ends_with"),
      ASTERIA_BINDING_BEGIN("std.string.ends_with", self, global, reader) {
        V_string text;
        V_string sufx;

        reader.start_overload();
        reader.required(text);     // text
        reader.required(sufx);     // suffix
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_ends_with, text, sufx);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("find"),
      ASTERIA_BINDING_BEGIN("std.string.find", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string patt;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find, text, 0, nullopt, patt);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find, text, from, nullopt, patt);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find, text, from, len, patt);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("rfind"),
      ASTERIA_BINDING_BEGIN("std.string.rfind", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string patt;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_rfind, text, 0, nullopt, patt);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_rfind, text, from, nullopt, patt);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_rfind, text, from, len, patt);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("find_and_replace"),
      ASTERIA_BINDING_BEGIN("std.string.find_and_replace", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string patt;
        V_string rep;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(patt);     // pattern
        reader.required(rep);     // replacement
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find_and_replace, text, 0, nullopt, patt, rep);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(patt);     // pattern
        reader.required(rep);     // replacement
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find_and_replace, text, from, nullopt, patt, rep);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(patt);     // pattern
        reader.required(rep);     // replacement
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find_and_replace, text, from, len, patt, rep);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("find_any_of"),
      ASTERIA_BINDING_BEGIN("std.string.find_any_of", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string acc;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(acc);      // accept
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find_any_of, text, 0, nullopt, acc);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(acc);      // accept
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find_any_of, text, from, nullopt, acc);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(acc);      // accept
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find_any_of, text, from, len, acc);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("rfind_any_of"),
      ASTERIA_BINDING_BEGIN("std.string.rfind_any_of", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string acc;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(acc);      // accept
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_rfind_any_of, text, 0, nullopt, acc);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(acc);      // accept
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_rfind_any_of, text, from, nullopt, acc);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(acc);      // accept
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_rfind_any_of, text, from, len, acc);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("find_not_of"),
      ASTERIA_BINDING_BEGIN("std.string.find_not_of", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string rej;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(rej);      // reject
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find_not_of, text, 0, nullopt, rej);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(rej);      // reject
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find_not_of, text, from, nullopt, rej);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(rej);      // reject
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_find_not_of, text, from, len, rej);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("rfind_not_of"),
      ASTERIA_BINDING_BEGIN("std.string.rfind_not_of", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string rej;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(rej);      // reject
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_rfind_not_of, text, 0, nullopt, rej);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(rej);      // reject
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_rfind_not_of, text, from, nullopt, rej);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(rej);      // reject
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_rfind_not_of, text, from, len, rej);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("reverse"),
      ASTERIA_BINDING_BEGIN("std.string.reverse", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_reverse, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("trim"),
      ASTERIA_BINDING_BEGIN("std.string.trim", self, global, reader) {
        V_string text;
        Opt_string rej;

        reader.start_overload();
        reader.required(text);    // text
        reader.optional(rej);    // [reject]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_trim, text, rej);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("triml"),
      ASTERIA_BINDING_BEGIN("std.string.triml", self, global, reader) {
        V_string text;
        Opt_string rej;

        reader.start_overload();
        reader.required(text);    // text
        reader.optional(rej);     // [reject]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_triml, text, rej);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("trimr"),
      ASTERIA_BINDING_BEGIN("std.string.trimr", self, global, reader) {
        V_string text;
        Opt_string rej;

        reader.start_overload();
        reader.required(text);    // text
        reader.optional(rej);     // [reject]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_trimr, text, rej);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("padl"),
      ASTERIA_BINDING_BEGIN("std.string.padl", self, global, reader) {
        V_string text;
        V_integer len;
        Opt_string pad;

        reader.start_overload();
        reader.required(text);    // text
        reader.required(len);     // length
        reader.optional(pad);     // [padding]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_padl, text, len, pad);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("padr"),
      ASTERIA_BINDING_BEGIN("std.string.padr", self, global, reader) {
        V_string text;
        V_integer len;
        Opt_string pad;

        reader.start_overload();
        reader.required(text);    // text
        reader.required(len);     // length
        reader.optional(pad);     // [padding]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_padr, text, len, pad);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("to_upper"),
      ASTERIA_BINDING_BEGIN("std.string.to_upper", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_to_upper, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("to_lower"),
      ASTERIA_BINDING_BEGIN("std.string.to_lower", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_to_lower, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("translate"),
      ASTERIA_BINDING_BEGIN("std.string.translate", self, global, reader) {
        V_string text;
        V_string in;
        Opt_string out;

        reader.start_overload();
        reader.required(text);    // text
        reader.required(in);      // input
        reader.optional(out);     // [output]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_translate, text, in, out);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("explode"),
      ASTERIA_BINDING_BEGIN("std.string.explode", self, global, reader) {
        V_string text;
        Opt_string delim;
        Opt_integer limit;

        reader.start_overload();
        reader.required(text);    // text
        reader.optional(delim);   // [delim]
        reader.optional(limit);   // [limit]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_explode, text, delim, limit);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("implode"),
      ASTERIA_BINDING_BEGIN("std.string.implode", self, global, reader) {
        V_array segs;
        Opt_string delim;

        reader.start_overload();
        reader.required(segs);    // segments
        reader.optional(delim);   // [delim]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_implode, segs, delim);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("hex_encode"),
      ASTERIA_BINDING_BEGIN("std.string.hex_encode", self, global, reader) {
        V_string data;
        Opt_boolean lowc;
        Opt_string delim;

        reader.start_overload();
        reader.required(data);    // data
        reader.optional(lowc);    // [lowercase]
        reader.optional(delim);   // [delim]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_hex_encode, data, lowc, delim);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("hex_decode"),
      ASTERIA_BINDING_BEGIN("std.string.hex_decode", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_hex_decode, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("base32_encode"),
      ASTERIA_BINDING_BEGIN("std.string.base32_encode", self, global, reader) {
        V_string data;
        Opt_boolean lowc;

        reader.start_overload();
        reader.required(data);    // data
        reader.optional(lowc);    // [lowercase]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_base32_encode, data, lowc);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("base32_decode"),
      ASTERIA_BINDING_BEGIN("std.string.base32_decode", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_base32_decode, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("base64_encode"),
      ASTERIA_BINDING_BEGIN("std.string.base64_encode", self, global, reader) {
        V_string data;

        reader.start_overload();
        reader.required(data);    // data
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_base64_encode, data);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("base64_decode"),
      ASTERIA_BINDING_BEGIN("std.string.base64_decode", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_base64_decode, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("url_encode"),
      ASTERIA_BINDING_BEGIN("std.string.url_encode", self, global, reader) {
        V_string data;
        Opt_boolean lowc;

        reader.start_overload();
        reader.required(data);    // data
        reader.optional(lowc);    // [lowercase]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_url_encode, data, lowc);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("url_decode"),
      ASTERIA_BINDING_BEGIN("std.string.url_decode", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_url_decode, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("url_encode_query"),
      ASTERIA_BINDING_BEGIN("std.string.url_encode_query", self, global, reader) {
        V_string data;
        Opt_boolean lowc;

        reader.start_overload();
        reader.required(data);    // data
        reader.optional(lowc);    // [lowercase]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_url_encode_query, data, lowc);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("url_decode_query"),
      ASTERIA_BINDING_BEGIN("std.string.url_decode_query", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_url_decode_query, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("utf8_validate"),
      ASTERIA_BINDING_BEGIN("std.string.utf8_validate", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // text
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_utf8_validate, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("utf8_encode"),
      ASTERIA_BINDING_BEGIN("std.string.utf8_encode", self, global, reader) {
        V_integer cp;
        V_array cps;
        Opt_boolean perm;

        reader.start_overload();
        reader.required(cp);      // code_point
        reader.optional(perm);    // [permissive]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_utf8_encode, cp, perm);

        reader.start_overload();
        reader.required(cps);     // code_points
        reader.optional(perm);    // [permissive]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_utf8_encode, cps, perm);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("utf8_decode"),
      ASTERIA_BINDING_BEGIN("std.string.utf8_decode", self, global, reader) {
        V_string text;
        Opt_boolean perm;

        reader.start_overload();
        reader.required(text);    // text
        reader.optional(perm);    // [permissive]
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_utf8_decode, text, perm);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pack_8"),
      ASTERIA_BINDING_BEGIN("std.string.pack_8", self, global, reader) {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_8, val);

        reader.start_overload();
        reader.required(vals);    // values
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_8, vals);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("unpack_8"),
      ASTERIA_BINDING_BEGIN("std.string.unpack_8", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_unpack_8, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pack_16be"),
      ASTERIA_BINDING_BEGIN("std.string.pack_16be", self, global, reader) {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_16be, val);

        reader.start_overload();
        reader.required(vals);    // values
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_16be, vals);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("unpack_16be"),
      ASTERIA_BINDING_BEGIN("std.string.unpack_16be", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_unpack_16be, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pack_16le"),
      ASTERIA_BINDING_BEGIN("std.string.pack_16le", self, global, reader) {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_16le, val);

        reader.start_overload();
        reader.required(vals);    // values
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_16le, vals);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("unpack_16le"),
      ASTERIA_BINDING_BEGIN("std.string.unpack_16le", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_unpack_16le, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pack_32be"),
      ASTERIA_BINDING_BEGIN("std.string.pack_32be", self, global, reader) {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_32be, val);

        reader.start_overload();
        reader.required(vals);    // values
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_32be, vals);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("unpack_32be"),
      ASTERIA_BINDING_BEGIN("std.string.unpack_32be", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_unpack_32be, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pack_32le"),
      ASTERIA_BINDING_BEGIN("std.string.pack_32le", self, global, reader) {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_32le, val);

        reader.start_overload();
        reader.required(vals);    // values
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_32le, vals);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("unpack_32le"),
      ASTERIA_BINDING_BEGIN("std.string.unpack_32le", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_unpack_32le, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pack_64be"),
      ASTERIA_BINDING_BEGIN("std.string.pack_64be", self, global, reader) {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_64be, val);

        reader.start_overload();
        reader.required(vals);    // values
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_64be, vals);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("unpack_64be"),
      ASTERIA_BINDING_BEGIN("std.string.unpack_64be", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_unpack_64be, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pack_64le"),
      ASTERIA_BINDING_BEGIN("std.string.pack_64le", self, global, reader) {
        V_integer val;
        V_array vals;

        reader.start_overload();
        reader.required(val);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_64le, val);

        reader.start_overload();
        reader.required(vals);    // values
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pack_64le, vals);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("unpack_64le"),
      ASTERIA_BINDING_BEGIN("std.string.unpack_64le", self, global, reader) {
        V_string text;

        reader.start_overload();
        reader.required(text);    // value
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_unpack_64le, text);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("format"),
      ASTERIA_BINDING_BEGIN("std.string.format", self, global, reader) {
        V_string templ;
        cow_vector<Value> args;

        reader.start_overload();
        reader.required(templ);         // template
        if(reader.end_overload(args))   // ...
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_format, templ, args);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pcre_find"),
      ASTERIA_BINDING_BEGIN("std.string.pcre_find", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string patt;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_find, text, 0, nullopt, patt);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_find, text, from, nullopt, patt);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_find, text, from, len, patt);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pcre_match"),
      ASTERIA_BINDING_BEGIN("std.string.pcre_match", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string patt;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_match, text, 0, nullopt, patt);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_match, text, from, nullopt, patt);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_match, text, from, len, patt);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pcre_named_match"),
      ASTERIA_BINDING_BEGIN("std.string.pcre_named_match", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string patt;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_named_match, text, 0, nullopt, patt);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_named_match, text, from, nullopt, patt);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(patt);     // pattern
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_named_match, text, from, len, patt);
      }
      ASTERIA_BINDING_END);

    result.insert_or_assign(::rocket::sref("pcre_replace"),
      ASTERIA_BINDING_BEGIN("std.string.pcre_replace", self, global, reader) {
        V_string text;
        V_integer from;
        Opt_integer len;
        V_string patt;
        V_string rep;

        reader.start_overload();
        reader.required(text);     // text
        reader.save_state(0);
        reader.required(patt);     // pattern
        reader.required(rep);     // replacement
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_replace, text, 0, nullopt, patt, rep);

        reader.load_state(0);      // text
        reader.required(from);     // from
        reader.save_state(0);
        reader.required(patt);     // pattern
        reader.required(rep);     // replacement
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_replace, text, from, nullopt, patt, rep);

        reader.load_state(0);      // text, from
        reader.optional(len);      // [length]
        reader.required(patt);     // pattern
        reader.required(rep);     // replacement
        if(reader.end_overload())
          ASTERIA_BINDING_RETURN_MOVE(self,
                    std_string_pcre_replace, text, from, len, patt, rep);
      }
      ASTERIA_BINDING_END);
  }

}  // namespace asteria
