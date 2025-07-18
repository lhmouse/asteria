// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "string.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/binding_generator.hpp"
#include "../utils.hpp"
#include <iconv.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
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
do_slice(const V_string& text, const optV_integer& from, const optV_integer& length)
  {
    // Behave like `::std::string::substr()` except that no exception is
    // thrown when `from` is greater than `text.size()`.
    int64_t sfrom = from.value_or(0);
    int64_t slen = text.ssize();
    if(sfrom >= slen)
      return ::std::make_pair(text.end(), text.end());

    // For positive offsets, return a subrange from `begin() + from`.
    if(sfrom >= 0)
      return do_slice(text, text.begin() + static_cast<ptrdiff_t>(sfrom), length);

    // Wrap `from` from the end. Notice that `from + slen` will not overflow
    // when `from` is negative and `slen` is not.
    int64_t rfrom = sfrom + slen;
    if(rfrom >= 0)
      return do_slice(text, text.begin() + static_cast<ptrdiff_t>(rfrom), length);

    // Get a subrange from the beginning of `text`, if the wrapped index is
    // before the first byte.
    if(!length)
      return ::std::make_pair(text.begin(), text.end());

    if(*length <= 0)
      return ::std::make_pair(text.begin(), text.begin());

    // Get a subrange excluding the part before the beginning.
    // Notice that `rfrom + *length` will not overflow when `rfrom` is
    // negative and `*length` is not.
    return do_slice(text, text.begin(), rfrom + *length);
  }

template<typename xIter>
class BMH_Searcher
  {
  private:
    xIter m_pbegin, m_pend;
    ptrdiff_t m_bcr_offsets[0x100];

  public:
    inline
    BMH_Searcher(xIter pbegin, xIter pend)
      :
        m_pbegin(pbegin), m_pend(pend)
      {
        const ptrdiff_t plen = this->m_pend - this->m_pbegin;
        if(plen <= 0)
          ASTERIA_THROW(("Empty pattern string not allowed"));

        // Create a table according to the Bad Character Rule.
        for(ptrdiff_t k = 0;  k != 0x100;  ++k)
          this->m_bcr_offsets[k] = plen;

        for(ptrdiff_t k = 0;  k != plen - 1;  ++k)
          this->m_bcr_offsets[uint8_t(this->m_pbegin[k])] = plen - 1 - k;
      }

  private:
    static
    uintptr_t
    do_xload_word(cow_string::const_iterator tcur)
      noexcept
      {
        uintptr_t btext;
        ::memcpy(&btext, &*tcur, sizeof(btext));
        return btext;
      }

    static
    uintptr_t
    do_xload_word(cow_string::const_reverse_iterator tcur)
      noexcept
      {
        uintptr_t btext;
        ::memcpy(&btext, &*tcur + 1 - sizeof(btext), sizeof(btext));
        return btext;
      }

  public:
    opt<xIter>
    search_opt(xIter tbegin, xIter tend)
      const
      {
        const ptrdiff_t plen = this->m_pend - this->m_pbegin;
        ROCKET_ASSERT(plen != 0);

        // If no enough bytes are given, there can't be matches.
        if(tend - tbegin < plen)
          return nullopt;

        // Perform a linear search for the first byte.
        // This has to be fast, but need not be very accurate.
        constexpr uintptr_t bmask = UINTPTR_MAX / 0xFF;
        constexpr ptrdiff_t bsize = sizeof(bmask);
        const uintptr_t bcomp = bmask * (uint8_t) this->m_pbegin[0];

        auto tcur = tbegin;
        const auto tfinalcand = tend - plen;

        while(tfinalcand - tcur >= bsize) {
          // Load a word and check whether it contains the first pattern byte.
          // Endianness does not matter.
          uintptr_t btext = this->do_xload_word(tcur) ^ bcomp;

          // Now see whether `btext` contains a zero byte. The condition
          // is that there shall be a byte whose MSB becomes one after the
          // subtraction below, but was zero before it.
          if((btext - bmask) & (btext ^ (bmask << 7)) & (bmask << 7))
            break;

          // Shift the read pointer past this word.
          tcur += bsize;
        }

        while(tfinalcand - tcur >= 0) {
          // Compare candidate intervals from right to left.
          ptrdiff_t tml = plen - 1;
          auto tcand = tcur;
          ptrdiff_t bcr_offset = this->m_bcr_offsets[uint8_t(tcand[tml])];

          while(tcand[tml] == this->m_pbegin[tml])
            if(--tml < 0)
              return tcand;  // found

          // Shift the read pointer by the proposed offset.
          tcur += bcr_offset;
        }

        // No matches has been found so far.
        return nullopt;
      }
  };

template<typename xIter>
BMH_Searcher<xIter>
do_create_searcher_for_pattern(xIter pbegin, xIter pend)
  {
    return BMH_Searcher<xIter>(pbegin, pend);
  }

template<typename xIter>
opt<xIter>
do_find_opt(xIter tbegin, xIter tend, xIter pbegin, xIter pend)
  {
    // If the pattern is empty, there is a match at the beginning.
    // Don't pass empty patterns to the BMH searcher.
    if(pbegin == pend)
      return tbegin;

    // If the text is empty but the pattern is not, there cannot be matches.
    if(tbegin == tend)
      return nullopt;

    // This is the slow path.
    const auto srch = do_create_searcher_for_pattern(pbegin, pend);
    return srch.search_opt(tbegin, tend);
  }

template<typename xIter>
opt<xIter>
do_find_of_opt(xIter begin, xIter end, const V_string& set, bool match)
  {
    bool table[256] = { };

    for(char c : set)
      table[(uint8_t)c] = true;

    for(auto it = begin;  it != end;  ++it)
      if(table[(uint8_t)*it] == match)
        return move(it);

    return nullopt;
  }

V_string
do_get_reject(const optV_string& reject)
  {
    if(!reject)
      return &" \t";

    return *reject;
  }

V_string
do_get_padding(const optV_string& padding)
  {
    if(!padding)
      return &" ";

    if(padding->empty())
      ASTERIA_THROW(("Empty padding string not valid"));

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

class PCRE2_Error
  {
  private:
    char m_buf[480];

  public:
    explicit
    PCRE2_Error(int err)
      noexcept
      {
        ::pcre2_get_error_message(err, (uint8_t*) this->m_buf, sizeof(m_buf));
      }

  public:
    const char*
    c_str()
      const noexcept
      { return this->m_buf;  }
  };

inline
tinyfmt&
operator<<(tinyfmt& fmt, const PCRE2_Error& err)
  {
    return fmt << err.c_str();
  }

class PCRE2_Matcher
  :
    public Abstract_Opaque
  {
  private:
    cow_string m_patt;
    uint32_t m_opts;
    unique_ptr<::pcre2_code, void (::pcre2_code*)> m_code;

    uint32_t m_name_count = 0;
    uint32_t m_name_size = 0;
    const uint8_t* m_name_table = nullptr;
    unique_ptr<::pcre2_match_data, void (::pcre2_match_data*)> m_match;

  public:
    PCRE2_Matcher(const V_string& patt, const optV_array& opts)
      :
        m_patt(patt), m_opts(PCRE2_NEVER_UTF | PCRE2_NEVER_UCP),
        m_code(::pcre2_code_free), m_match(::pcre2_match_data_free)
      {
        // Check options.
        if(opts)
          for(const auto& opt : *opts) {
            const auto& str = opt.as_string();
            if(str == "caseless")
              this->m_opts |= PCRE2_CASELESS;
            else if(str == "dotall")
              this->m_opts |= PCRE2_DOTALL;
            else if(str == "extended")
              this->m_opts |= PCRE2_EXTENDED;
            else if(str == "multiline")
              this->m_opts |= PCRE2_MULTILINE;
            else
              ASTERIA_THROW(("Invalid option for regular expression: $1"), str);
          }

        // Compile the regular expression.
        int err;
        size_t off;
        if(!this->m_code.reset(::pcre2_compile(
                                   reinterpret_cast<const uint8_t*>(this->m_patt.data()),
                                   this->m_patt.size(), this->m_opts, &err, &off, nullptr)))
          ASTERIA_THROW((
              "Invalid regular expression: $1",
              "[`pcre2_compile()` failed at offset `$2`: $3]"),
              this->m_patt, off, PCRE2_Error(err));

        this->do_initialize_match_data();
      }

    PCRE2_Matcher(const PCRE2_Matcher& other, int)
      :
        m_patt(other.m_patt), m_opts(other.m_opts),
        m_code(::pcre2_code_free), m_match(::pcre2_match_data_free)
      {
        // Copy the regular expression.
        if(!this->m_code.reset(::pcre2_code_copy(other.m_code)))
          ASTERIA_THROW((
              "Could not copy regular expression",
              "[`pcre2_code_copy()` failed]"));

        this->do_initialize_match_data();
      }

  private:
    void
    do_initialize_match_data()
      {
        ::pcre2_pattern_info(this->m_code, PCRE2_INFO_NAMECOUNT, &(this->m_name_count));
        if(this->m_name_count != 0) {
          ::pcre2_pattern_info(this->m_code, PCRE2_INFO_NAMEENTRYSIZE, &(this->m_name_size));
          ::pcre2_pattern_info(this->m_code, PCRE2_INFO_NAMETABLE, &(this->m_name_table));
        }

        if(!this->m_match.reset(::pcre2_match_data_create_from_pattern(this->m_code, nullptr)))
          ASTERIA_THROW((
              "Could not allocate `match_data` structure",
              "[`pcre2_match_data_create_from_pattern()` failed]"));
      }

    opt<size_t>
    do_pcre2_match_opt(const V_string& text, optV_integer from, optV_integer length)
      const
      {
        opt<size_t> result;
        auto range = do_slice(text, from, length);
        auto sub_off = static_cast<size_t>(range.first - text.begin());
        auto sub_ptr = reinterpret_cast<const uint8_t*>(text.data()) + sub_off;
        auto sub_len = static_cast<size_t>(range.second - range.first);

        // Try matching.
        int err = ::pcre2_match(this->m_code, sub_ptr, sub_len, 0, 0, this->m_match, nullptr);
        if((err < 0) && (err != PCRE2_ERROR_NOMATCH))
          ASTERIA_THROW((
              "Regular expression match failure",
              "[`pcre2_match()` failed: $1]"),
              PCRE2_Error(err));

        if(err != PCRE2_ERROR_NOMATCH)
          result.emplace(sub_off);

        return result;
      }

  public:
    tinyfmt&
    describe(tinyfmt& fmt)
      const override
      {
        return format(fmt, "instance of `std.string.PCRE` at `$1`", this);
      }

    void
    collect_variables(Variable_HashMap&, Variable_HashMap&)
      const override
      {
      }

    PCRE2_Matcher*
    clone_opt(refcnt_ptr<Abstract_Opaque>& out)
      const override
      {
        auto ptr = new PCRE2_Matcher(*this, 42);
        out.reset(ptr);
        return ptr;
      }

    opt<pair<V_integer, V_integer>>
    find(const V_string& text, optV_integer from, optV_integer length)
      const
      {
        auto sub_off = this->do_pcre2_match_opt(text, from, length);
        if(!sub_off)
          return nullopt;

        // This is copied from PCRE2 manual:
        //   If a pattern uses the \K escape sequence within a positive
        //   assertion, the reported start of a successful match can be
        //   greater than the end of the match. For example, if the
        //   pattern (?=ab\K) is matched against "ab", the start and end
        //   offset values for the match are 2 and 0.
        const size_t* ovec = ::pcre2_get_ovector_pointer(this->m_match);
        size_t off = *sub_off + ovec[0];
        size_t len = ::rocket::max(ovec[0], ovec[1]) - ovec[0];
        return {{ static_cast<ptrdiff_t>(off), static_cast<ptrdiff_t>(len) }};
      }

    optV_array
    match(const V_string& text, optV_integer from, optV_integer length)
      const
      {
        auto sub_off = this->do_pcre2_match_opt(text, from, length);
        if(!sub_off)
          return nullopt;

        // Compose the match result array.
        // The first element should be the matched substring. The others
        // are positional capturing groups. If a group matched nothing,
        // its corresponding element is `null`.
        const size_t* ovec = ::pcre2_get_ovector_pointer(this->m_match);
        size_t npairs = ::pcre2_get_ovector_count(this->m_match);
        V_array matches;
        matches.append(npairs);
        for(size_t k = 0;  k != npairs;  ++k) {
          const size_t* opair = ovec + k * 2;
          if(ROCKET_UNEXPECT(opair[0] == PCRE2_UNSET))
            continue;

          size_t off = *sub_off + opair[0];
          size_t len = ::rocket::max(opair[0], opair[1]) - opair[0];
          matches.mut(k) = cow_string(text, off, len);
        }
        return move(matches);
      }

    optV_object
    named_match(const V_string& text, optV_integer from, optV_integer length)
      const
      {
        auto sub_off = this->do_pcre2_match_opt(text, from, length);
        if(!sub_off)
          return nullopt;

        // Compose the match result object.
        const size_t* ovec = ::pcre2_get_ovector_pointer(this->m_match);
        V_object matches;
        matches.reserve(this->m_name_count + 1);
        for(size_t k = 0;  k != this->m_name_count;  ++k) {
          const uint8_t* entry = this->m_name_table + this->m_name_size * k;
          auto ins = matches.try_emplace(cow_string(reinterpret_cast<const char*>(entry + 2)));

          const size_t* opair = ovec + (entry[0] << 8 | entry[1]) * 2;
          if(ROCKET_UNEXPECT(opair[0] == PCRE2_UNSET))
            continue;

          size_t off = *sub_off + opair[0];
          size_t len = ::rocket::max(opair[0], opair[1]) - opair[0];
          ins.first->second = cow_string(text, off, len);
        }
        if(ovec[0] != PCRE2_UNSET) {
          size_t off = *sub_off + ovec[0];
          size_t len = ::rocket::max(ovec[0], ovec[1]) - ovec[0];
          matches.try_emplace(&"&", cow_string(text, off, len));
        }
        return move(matches);
      }

    V_string
    replace(const V_string& text, optV_integer from, optV_integer length, const V_string& rep)
      const
      {
        auto range = do_slice(text, from, length);
        auto sub_off = static_cast<size_t>(range.first - text.begin());
        auto sub_ptr = reinterpret_cast<const uint8_t*>(text.data()) + sub_off;
        auto sub_len = static_cast<size_t>(range.second - range.first);

        // Prepare the output buffer.
        size_t out_len = text.size();
        V_string out_str(out_len, '*');

        // Try substitution.
        int err = ::pcre2_substitute(this->m_code, sub_ptr, sub_len, 0,
                        this->m_opts | PCRE2_SUBSTITUTE_EXTENDED | PCRE2_SUBSTITUTE_GLOBAL
                                     | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH,
                        this->m_match, nullptr, reinterpret_cast<const uint8_t*>(rep.data()),
                        rep.size(), reinterpret_cast<uint8_t*>(out_str.mut_data()), &out_len);

        if(err == PCRE2_ERROR_NOMEMORY) {
          // The output length should have been written to `out_len`.
          // Resize the buffer and try again.
          out_str.assign(out_len, '/');
          err = ::pcre2_substitute(this->m_code, sub_ptr, sub_len, 0,
                        this->m_opts | PCRE2_SUBSTITUTE_EXTENDED | PCRE2_SUBSTITUTE_GLOBAL,
                        this->m_match, nullptr, reinterpret_cast<const uint8_t*>(rep.data()),
                        rep.size(), reinterpret_cast<uint8_t*>(out_str.mut_data()), &out_len);
        }

        if(err < 0)
          ASTERIA_THROW((
              "Regular expression substitution failure",
              "[`pcre2_substitute()` failed: $1]"),
              PCRE2_Error(err));

        // Discard excess characters.
        ROCKET_ASSERT(out_len <= out_str.size());
        out_str.erase(out_len);

        // Concatenate it with unreplaced parts.
        out_str.insert(out_str.begin(), text.begin(), range.first);
        out_str.insert(out_str.end(), range.second, text.end());
        return out_str;
      }
  };

void
do_construct_PCRE(V_object& result, V_string pattern, optV_array options)
  {
    static constexpr auto s_private_uuid = &"{ae01217a-ea1a-4694-8bc0-bf607ed34f77}";
    result.insert_or_assign(s_private_uuid, std_string_PCRE_private(pattern, options));

    result.insert_or_assign(&"find",
      ASTERIA_BINDING(
        "std.string.PCRE::find", "text, [from, [length]]",
        Reference&& self, Argument_Reader&& reader)
      {
        const V_opaque* matcher = nullptr;
        const Value& self_val = self.dereference_readonly();
        if(self_val.is_object())
          if(auto pval = self_val.as_object().ptr(s_private_uuid))
            if(pval->is_opaque())
              matcher = &(pval->as_opaque());

        if(!matcher)
          ASTERIA_THROW(("Invalid member function: type validation failure"));

        V_string text;
        optV_integer from, len;
        optV_array opts;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        if(reader.end_overload())
          return (Value) std_string_PCRE_find(*matcher, text, 0, nullopt);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        if(reader.end_overload())
          return (Value) std_string_PCRE_find(*matcher, text, from, nullopt);

        reader.load_state(1);
        reader.optional(len);
        if(reader.end_overload())
          return (Value) std_string_PCRE_find(*matcher, text, from, len);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"match",
      ASTERIA_BINDING(
        "std.string.PCRE::match", "text, [from, [length]]",
        Reference&& self, Argument_Reader&& reader)
      {
        const V_opaque* matcher = nullptr;
        const Value& self_val = self.dereference_readonly();
        if(self_val.is_object())
          if(auto pval = self_val.as_object().ptr(s_private_uuid))
            if(pval->is_opaque())
              matcher = &(pval->as_opaque());

        if(!matcher)
          ASTERIA_THROW(("Invalid member function: type validation failure"));

        V_string text;
        optV_integer from, len;
        optV_array opts;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        if(reader.end_overload())
          return (Value) std_string_PCRE_match(*matcher, text, 0, nullopt);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        if(reader.end_overload())
          return (Value) std_string_PCRE_match(*matcher, text, from, nullopt);

        reader.load_state(1);
        reader.optional(len);
        if(reader.end_overload())
          return (Value) std_string_PCRE_match(*matcher, text, from, len);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"named_match",
      ASTERIA_BINDING(
        "std.string.PCRE::named_match", "text, [from, [length]]",
        Reference&& self, Argument_Reader&& reader)
      {
        const V_opaque* matcher = nullptr;
        const Value& self_val = self.dereference_readonly();
        if(self_val.is_object())
          if(auto pval = self_val.as_object().ptr(s_private_uuid))
            if(pval->is_opaque())
              matcher = &(pval->as_opaque());

        if(!matcher)
          ASTERIA_THROW(("Invalid member function: type validation failure"));

        V_string text;
        optV_integer from, len;
        optV_array opts;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        if(reader.end_overload())
          return (Value) std_string_PCRE_named_match(*matcher, text, 0, nullopt);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        if(reader.end_overload())
          return (Value) std_string_PCRE_named_match(*matcher, text, from, nullopt);

        reader.load_state(1);
        reader.optional(len);
        if(reader.end_overload())
          return (Value) std_string_PCRE_named_match(*matcher, text, from, len);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"replace",
      ASTERIA_BINDING(
        "std.string.PCRE::replace", "text, [from, [length]], replacement",
        Reference&& self, Argument_Reader&& reader)
      {
        const V_opaque* matcher = nullptr;
        const Value& self_val = self.dereference_readonly();
        if(self_val.is_object())
          if(auto pval = self_val.as_object().ptr(s_private_uuid))
            if(pval->is_opaque())
              matcher = &(pval->as_opaque());

        if(!matcher)
          ASTERIA_THROW(("Invalid member function: type validation failure"));

        V_string text, rep;
        optV_integer from, len;
        optV_array opts;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(rep);
        if(reader.end_overload())
          return (Value) std_string_PCRE_replace(*matcher, text, 0, nullopt, rep);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(rep);
        if(reader.end_overload())
          return (Value) std_string_PCRE_replace(*matcher, text, from, nullopt, rep);

        reader.load_state(1);
        reader.optional(len);
        reader.required(rep);
        if(reader.end_overload())
          return (Value) std_string_PCRE_replace(*matcher, text, from, len, rep);

        reader.throw_no_matching_function_call();
      });
  }

struct iconv_closer
  {
    using handle_type  = ::iconv_t;
    using closer_type  = decltype(::iconv_close)*;

    handle_type
    null()
      const noexcept
      { return (::iconv_t)-1;  }

    bool
    is_null(handle_type cd)
      const noexcept
      { return cd == (::iconv_t)-1;  }

    int
    close(handle_type cd)
      const noexcept
      { return ::iconv_close(cd);  }
  };

}  // namespace

V_string
std_string_slice(V_string text, optV_integer from, optV_integer length)
  {
    // Use reference counting as our advantage.
    V_string res = text;
    auto range = do_slice(res, from, length);
    if(range.second - range.first != res.ssize())
      res.assign(range.first, range.second);
    return res;
  }

V_string
std_string_replace_slice(V_string text, optV_integer from, optV_integer length, V_string replacement,
                         optV_integer rfrom, optV_integer rlength)
  {
    V_string res = text;
    auto range = do_slice(res, from, length);
    auto rep_range = do_slice(replacement, rfrom.value_or(0), rlength);

    // Replace the subrange.
    res.replace(range.first, range.second, rep_range.first, rep_range.second);
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
    return text1.substr_compare(0, static_cast<size_t>(*length), text2, 0, static_cast<size_t>(*length));
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
std_string_find(V_string text, optV_integer from, optV_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_opt(range.first, range.second, pattern.begin(), pattern.end());
    return qit ? (*qit - text.begin()) : optV_integer();
  }

optV_integer
std_string_rfind(V_string text, optV_integer from, optV_integer length, V_string pattern)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_opt(::std::make_reverse_iterator(range.second),
                           ::std::make_reverse_iterator(range.first),
                           pattern.rbegin(), pattern.rend());
    return qit ? (text.rend() - *qit - pattern.ssize()) : optV_integer();
  }

V_string
std_string_replace(V_string text, optV_integer from, optV_integer length, V_string pattern,
                   V_string replacement)
  {
    auto range = do_slice(text, from, length);
    if(range.first == range.second)
      return text;

    V_string res;
    res.append(text.begin(), range.first);

    if(pattern.empty()) {
      // Insert `replacement` beside all bytes.
      while(range.first != range.second) {
        res.append(replacement);
        res.push_back(*(range.first));
        ++ range.first;
      }
      res.append(replacement);
      res.append(range.second, text.end());
    }
    else {
      // Search for `pattern` in `text`.
      opt<cow_string::const_iterator> qbrk;
      const auto srch = do_create_searcher_for_pattern(pattern.begin(), pattern.end());
      while(!!(qbrk = srch.search_opt(range.first, range.second))) {
        res.append(range.first, *qbrk);
        res.append(replacement);
        range.first = *qbrk + pattern.ssize();
      }
      res.append(range.first, text.end());
    }
    return res;
  }

optV_integer
std_string_find_any_of(V_string text, optV_integer from, optV_integer length, V_string accept)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(range.first, range.second, accept, true);
    return qit ? (*qit - text.begin()) : optV_integer();
  }

optV_integer
std_string_find_not_of(V_string text, optV_integer from, optV_integer length, V_string reject)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(range.first, range.second, reject, false);
    return qit ? (*qit - text.begin()) : optV_integer();
  }

optV_integer
std_string_rfind_any_of(V_string text, optV_integer from, optV_integer length, V_string accept)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(::std::make_reverse_iterator(range.second),
                              ::std::make_reverse_iterator(range.first), accept, true);
    return qit ? (text.rend() - *qit - 1) : optV_integer();
  }

optV_integer
std_string_rfind_not_of(V_string text, optV_integer from, optV_integer length, V_string reject)
  {
    auto range = do_slice(text, from, length);
    auto qit = do_find_of_opt(::std::make_reverse_iterator(range.second),
                              ::std::make_reverse_iterator(range.first), reject, false);
    return qit ? (text.rend() - *qit - 1) : optV_integer();
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
    size_t bpos = text.find_not_of(rchars);
    if(bpos == V_string::npos)
      return &"";

    // Get the index of the last byte to keep.
    size_t epos = text.rfind_not_of(rchars) + 1;
    if((bpos == 0) && (epos == text.size()))
      return text;

    // Return the remaining part of `text`.
    return text.substr(bpos, epos - bpos);
  }

V_string
std_string_triml(V_string text, optV_string reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0)
      return text;

    // Get the index of the first byte to keep.
    size_t bpos = text.find_not_of(rchars);
    if(bpos == V_string::npos)
      return &"";
    else if(bpos == 0)
      return text;

    // Return the remaining part of `text`.
    return text.substr(bpos);
  }

V_string
std_string_trimr(V_string text, optV_string reject)
  {
    auto rchars = do_get_reject(reject);
    if(rchars.length() == 0)
      return text;

    // Get the index of the last byte to keep.
    size_t epos = text.rfind_not_of(rchars) + 1;
    if(epos == 0)
      return &"";
    else if(epos == text.size())
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

      if(outputs && (ipos < outputs->size()))
        wptr[i] = outputs->at(ipos);
      else
        res.erase(i--, 1);
    }
    return res;
  }

V_array
std_string_explode(V_string text, optV_string delim, optV_integer limit)
  {
    uint64_t rlimit = UINT64_MAX;
    if(limit) {
      if(*limit <= 0)
        ASTERIA_THROW((
            "Max number of segments must be positive (limit `$1`)"),
            *limit);

      rlimit = static_cast<uint64_t>(*limit);
    }

    // Return an empty array if the string is empty.
    V_array segments;
    auto tcur = text.begin();

    if(tcur == text.end())
      return segments;

    if(!delim || delim->empty()) {
      // Split the string into bytes.
      while((segments.size() + 1 < rlimit) && (tcur != text.end())) {
        // Store a reference to the null-terminated string allocated statically.
        // Don't bother allocating a new buffer of only two characters.
        segments.emplace_back(::rocket::sref(s_char_table[uint8_t(*tcur)]));
        tcur += 1;
      }
      if(tcur != text.end())
        segments.emplace_back(V_string(tcur, text.end()));
    }
    else {
      // Search for `*delim` in the `text`.
      opt<cow_string::const_iterator> qbrk;
      const auto srch = do_create_searcher_for_pattern(delim->begin(), delim->end());
      while((segments.size() + 1 < rlimit) && !!(qbrk = srch.search_opt(tcur, text.end()))) {
        // Push this segment and move `tcur` past it.
        segments.emplace_back(V_string(tcur, *qbrk));
        tcur = *qbrk + delim->ssize();
      }
      segments.emplace_back(V_string(tcur, text.end()));
    }
    return segments;
  }

V_string
std_string_implode(V_array segments, optV_string delim)
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
std_string_hex_encode(V_string data, optV_string delim)
  {
    const char* pdelim = delim ? delim->data() : "";
    size_t ndelim = delim ? delim->size() : 0;

    V_string text;
    text.reserve(data.size() * (2 + ndelim));

    // These shall be operated in big-endian order.
    uint32_t reg = 0;

    // Encode source data.
    size_t nread = 0;
    while(nread != data.size()) {
      // Insert a delimiter before every byte other than the first one.
      if(!text.empty())
        text.append(pdelim, ndelim);

      // Read a byte.
      reg = data[nread++] & 0xFF;
      reg <<= 24;

      // Encode it.
      for(size_t i = 0;  i < 2;  ++i) {
        uint32_t b = (reg >> 27) & 0xFE;
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
          ASTERIA_THROW(("Unpaired hexadecimal digit"));

        continue;
      }
      reg <<= 4;

      // Decode a digit.
      pos = do_xstrchr(s_base16_table, c);
      if(!pos)
        ASTERIA_THROW(("Invalid hexadecimal digit (character `$1`)"), c);

      reg |= static_cast<uint32_t>(pos - s_base16_table) / 2;

      // Decode the current group if it is complete.
      if(!(reg & 0x1'00))
        continue;

      data += static_cast<char>(reg);
      reg = 1;
    }
    if(reg != 1)
      ASTERIA_THROW(("Unpaired hexadecimal digit"));

    return data;
  }

V_string
std_string_base32_encode(V_string data)
  {
    V_string text;
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
        uint32_t b = (reg >> 58) & 0xFE;
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
        uint32_t b = (reg >> 58) & 0xFE;
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
          ASTERIA_THROW(("Incomplete base32 group"));

        continue;
      }
      reg <<= 5;

      if(c == s_base32_table[64]) {
        // The character is a padding character.
        if(reg < 0x100)
          ASTERIA_THROW(("Unexpected base32 padding character"));

        npad += 1;
      }
      else {
        // Decode a digit.
        pos = do_xstrchr(s_base32_table, c);
        if(!pos)
          ASTERIA_THROW(("Invalid base32 digit (character `$1`)"), c);

        if(npad != 0)
          ASTERIA_THROW(("Unexpected base32 digit following padding character"));

        reg |= static_cast<uint32_t>(pos - s_base32_table) / 2;
      }

      // Decode the current group if it is complete.
      if(!(reg & 0x1'00'00'00'00'00))
        continue;

      size_t m = (40 - npad * 5) / 8;
      size_t p = (m * 8 + 4) / 5;
      if(p + npad != 8)
        ASTERIA_THROW((
            "Unexpected number of base32 padding characters (got `$1`)"),
            npad);

      for(size_t i = 0; i < m; ++i) {
        reg <<= 8;
        data += static_cast<char>(reg >> 40);
      }
      reg = 1;
      npad = 0;
    }
    if(reg != 1)
      ASTERIA_THROW(("Incomplete base32 group"));

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
          ASTERIA_THROW(("Incomplete base64 group"));

        continue;
      }
      reg <<= 6;

      if(c == s_base64_table[64]) {
        // The character is a padding character.
        if(reg < 0x100)
          ASTERIA_THROW(("Unexpected base64 padding character"));

        npad += 1;
      }
      else {
        // Decode a digit.
        pos = do_xstrchr(s_base64_table, c);
        if(!pos)
          ASTERIA_THROW(("Invalid base64 digit (character `$1`)"), c);

        if(npad != 0)
          ASTERIA_THROW((
              "Unexpected base64 digit following padding character"));

        reg |= static_cast<uint32_t>(pos - s_base64_table);
      }

      // Decode the current group if it is complete.
      if(!(reg & 0x1'00'00'00))
        continue;

      size_t m = (24 - npad * 6) / 8;
      size_t p = (m * 8 + 5) / 6;
      if(p + npad != 4)
        ASTERIA_THROW((
            "Unexpected number of base64 padding characters (got `$1`)"),
            npad);

      for(size_t i = 0; i < m; ++i) {
        reg <<= 8;
        data += static_cast<char>(reg >> 24);
      }
      reg = 1;
      npad = 0;
    }
    if(reg != 1)
      ASTERIA_THROW(("Incomplete base64 group"));

    return data;
  }

V_string
std_string_url_encode(V_string data)
  {
    // Only modify the string as needed, without causing copies on write.
    V_string text = data;
    size_t nread = 0;
    while(nread != text.size()) {
      char c = text[nread++];

      // Check for characters that don't need escaping.
      if(((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')))
        continue;

      if((c == '-') || (c == '_') || (c == '~') || (c == '.'))
        continue;

      // Escape it.
      uint32_t hi = static_cast<uint32_t>((c & 0xF0) >> 3);
      uint32_t lo = static_cast<uint32_t>((c & 0x0F) << 1);
      char pseq[3] = { '%', s_base16_table[hi], s_base16_table[lo] };
      text.replace(nread - 1, 1, pseq, 3);
      nread += 2;
    }
    return text;
  }

V_string
std_string_url_decode(V_string text)
  {
    // Only modify the string as needed, without causing copies on write.
    V_string data = text;
    size_t nread = 0;
    while(nread != data.size()) {
      char c = data[nread++];

      // Check for control characters and spaces.
      if(((c >= 0) && (c <= ' ')) || (c == '\x7F') || (c == '\xFF'))
        ASTERIA_THROW(("Invalid character in URL (character `$1`)"), (int) c);

      // Check for a percent sign followed by two hexadecimal characters.
      // Everything else is left intact.
      if(c != '%')
        continue;

      if(data.size() - nread < 2)
        ASTERIA_THROW(("No enough hexadecimal digits after `%`"));

      // Parse the first digit.
      c = data[nread++];
      const char* pos_hi = do_xstrchr(s_base16_table, c);
      if(!pos_hi)
        ASTERIA_THROW(("Invalid hexadecimal digit (character `$1`)"), c);

      // Parse the second digit.
      c = data[nread++];
      auto pos_lo = do_xstrchr(s_base16_table, c);
      if(!pos_lo)
        ASTERIA_THROW(("Invalid hexadecimal digit (character `$1`)"), c);

      // Compose the byte.
      ptrdiff_t value = (pos_hi - s_base16_table) / 2 * 16;
      value += (pos_lo - s_base16_table) / 2;

      // Replace this sequence with the decoded byte.
      nread -= 2;
      data.replace(nread - 1, 3, 1, static_cast<char>(value));
    }
    return data;
  }

V_string
std_string_url_query_encode(V_string data)
  {
    // Only modify the string as needed, without causing copies on write.
    V_string text = data;
    size_t nread = 0;
    while(nread != text.size()) {
      char c = text[nread++];

      // Check for characters that don't need escaping.
      if(((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')))
        continue;

      if((c == '-') || (c == '_') || (c == '.'))
        continue;

      // Encode spaces specially.
      if(c == ' ') {
        text.mut(nread - 1) = '+';
        continue;
      }

      // Escape it.
      uint32_t hi = static_cast<uint32_t>((c & 0xF0) >> 3);
      uint32_t lo = static_cast<uint32_t>((c & 0x0F) << 1);
      char pseq[3] = { '%', s_base16_table[hi], s_base16_table[lo] };
      text.replace(nread - 1, 1, pseq, 3);
      nread += 2;
    }
    return text;
  }

V_string
std_string_url_query_decode(V_string text)
  {
    // Only modify the string as needed, without causing copies on write.
    V_string data = text;
    size_t nread = 0;
    while(nread != data.size()) {
      char c = data[nread++];

      // Check for control characters and spaces.
      if(((c >= 0) && (c <= ' ')) || (c == '\x7F') || (c == '\xFF'))
        ASTERIA_THROW(("Invalid character in URL (character `$1`)"), (int) c);

      // Decode spaces specially.
      if(c == '+') {
        data.mut(nread - 1) = ' ';
        continue;
      }

      // Check for a percent sign followed by two hexadecimal characters.
      // Everything else is left intact.
      if(c != '%')
        continue;

      // Two hexadecimal characters shall follow.
      if(data.size() - nread < 2)
        ASTERIA_THROW(("No enough hexadecimal digits after `%`"));

      // Parse the first digit.
      c = data[nread++];
      const char* pos_hi = do_xstrchr(s_base16_table, c);
      if(!pos_hi)
        ASTERIA_THROW(("Invalid hexadecimal digit (character `$1`)"), c);

      // Parse the second digit.
      c = data[nread++];
      auto pos_lo = do_xstrchr(s_base16_table, c);
      if(!pos_lo)
        ASTERIA_THROW(("Invalid hexadecimal digit (character `$1`)"), c);

      // Compose the byte.
      ptrdiff_t value = (pos_hi - s_base16_table) / 2 * 16;
      value += (pos_lo - s_base16_table) / 2;

      // Replace this sequence with the decoded byte.
      nread -= 2;
      data.replace(nread - 1, 3, 1, static_cast<char>(value));
    }
    return data;
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
    auto cp = ::rocket::clamp_cast<char32_t>(code_point, -1, INT32_MAX);
    if(!utf8_encode(text, cp)) {
      // This comparison with `true` is by intention, because it may be unset.
      if(permissive != true)
        ASTERIA_THROW(("Invalid UTF code point (value `$1`)"), code_point);

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
      auto cp = ::rocket::clamp_cast<char32_t>(value, -1, INT32_MAX);
      if(!utf8_encode(text, cp)) {
        // This comparison with `true` is by intention, because it may be unset.
        if(permissive != true)
          ASTERIA_THROW(("Invalid UTF code point (value `$1`)"), value);

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
          ASTERIA_THROW(("Invalid UTF-8 string"));

        cp = text[offset++] & 0xFF;
      }
      code_points.emplace_back(V_integer(cp));
    }
    return code_points;
  }

V_string
std_string_format(V_string templ, cow_vector<Value> values)
  {
    // Prepare inserters.
    cow_vector<::rocket::formatter> insts;
    insts.reserve(values.size());
    for(const auto& val : values)
      insts.push_back({
        [](tinyfmt& fmt, const void* ptr) {
          const auto& r = *(const Value*) ptr;
          if(r.is_string())
            fmt << r.as_string();
          else
            r.print_to(fmt);
        },
        &val });

    // Compose the string into a stream.
    ::rocket::tinyfmt_str fmt;
    formatv(fmt, templ.safe_c_str(), insts.data(), insts.size());
    return fmt.extract_string();
  }

V_object
std_string_PCRE(V_string pattern, optV_array options)
  {
    V_object result;
    do_construct_PCRE(result, pattern, options);
    return result;
  }

V_opaque
std_string_PCRE_private(V_string pattern, optV_array options)
  {
    return ::rocket::make_refcnt<PCRE2_Matcher>(pattern, options);
  }

opt<pair<V_integer, V_integer>>
std_string_PCRE_find(const V_opaque& pcre, V_string text, optV_integer from, optV_integer length)
  {
    return pcre.get<PCRE2_Matcher>().find(text, from, length);
  }

optV_array
std_string_PCRE_match(const V_opaque& pcre, V_string text, optV_integer from, optV_integer length)
  {
    return pcre.get<PCRE2_Matcher>().match(text, from, length);
  }

optV_object
std_string_PCRE_named_match(const V_opaque& pcre, V_string text, optV_integer from, optV_integer length)
  {
    return pcre.get<PCRE2_Matcher>().named_match(text, from, length);
  }

V_string
std_string_PCRE_replace(const V_opaque& pcre, V_string text, optV_integer from, optV_integer length,
                        V_string replacement)
  {
    return pcre.get<PCRE2_Matcher>().replace(text, from, length, replacement);
  }

opt<pair<V_integer, V_integer>>
std_string_pcre_find(V_string text, optV_integer from, optV_integer length, V_string pattern,
                     optV_array options)
  {
    PCRE2_Matcher m(pattern, options);
    return m.find(text, from, length);
  }

optV_array
std_string_pcre_match(V_string text, optV_integer from, optV_integer length, V_string pattern,
                      optV_array options)
  {
    PCRE2_Matcher m(pattern, options);
    return m.match(text, from, length);
  }

optV_object
std_string_pcre_named_match(V_string text, optV_integer from, optV_integer length, V_string pattern,
                            optV_array options)
  {
    PCRE2_Matcher m(pattern, options);
    return m.named_match(text, from, length);
  }

V_string
std_string_pcre_replace(V_string text, optV_integer from, optV_integer length, V_string pattern,
                        V_string replacement, optV_array options)
  {
    PCRE2_Matcher m(pattern, options);
    return m.replace(text, from, length, replacement);
  }

V_string
std_string_iconv(V_string to_encoding, V_string text, optV_string from_encoding)
  {
    const char* to_enc = to_encoding.safe_c_str();
    const char* from_enc = "UTF-8";
    if(from_encoding)
      from_enc = from_encoding->safe_c_str();

    // Create the descriptor.
    ::rocket::unique_handle<::iconv_t, iconv_closer> qcd;
    if(!qcd.reset(::iconv_open(to_enc, from_enc)))
      ASTERIA_THROW((
             "Could not create iconv context to encoding `$1` from `$2`"),
             "[`iconv_open()` failed: ${errno:full}]",
             to_enc, from_enc);

    // Perform bytewise conversion.
    V_string output;
    char temp[64];
    const char* inp = text.c_str();
    size_t inc = text.size();

    while(inc != 0) {
      char* outp = temp;
      size_t outc = sizeof(temp);

      size_t r = ::iconv(qcd, const_cast<char**>(&inp), &inc, &outp, &outc);
      output.append(temp, outp);
      if((r == (size_t)-1) && (errno != E2BIG))
        ASTERIA_THROW((
               "Invalid input byte to encoding `$1` from `$2` at offset `$3`",
               "[`iconv()` failed: ${errno:full}]"),
               to_enc, from_enc, inp - text.c_str());
    }
    return output;
  }

V_integer
std_string_visual_width(V_string text)
  {
#if defined __CYGWIN__
    ASTERIA_THROW(("`std.string.visual_width()` is not implemented in Cygwin or MSYS2"));
#else
    V_integer width = 0;

    size_t offset = 0;
    while(offset < text.size()) {
      // Try decoding a code point.
      char32_t cp;
      if(!utf8_decode(cp, text, offset))
        ASTERIA_THROW(("Invalid UTF-8 string"));

      int w = ::wcwidth(static_cast<wchar_t>(cp));
      if(w < 0)
        ASTERIA_THROW(("Non-printable character encountered"));

      width += w;
    }
    return width;
#endif
  }

void
create_bindings_string(V_object& result, API_Version version)
  {
    result.insert_or_assign(&"slice",
      ASTERIA_BINDING(
        "std.string.slice", "text, from, [length]",
        Argument_Reader&& reader)
      {
        V_string text;
        V_integer from;
        optV_integer len;

        reader.start_overload();
        reader.required(text);
        reader.required(from);
        reader.optional(len);
        if(reader.end_overload())
          return (Value) std_string_slice(text, from, len);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"replace_slice",
      ASTERIA_BINDING(
        "std.string.replace_slice", "text, from, [length], replacement, [rfrom, [rlength]]",
        Argument_Reader&& reader)
      {
        V_string text;
        V_integer from;
        optV_integer len, rfrom, rlen;
        V_string rep;

        reader.start_overload();
        reader.required(text);
        reader.required(from);
        reader.save_state(0);
        reader.required(rep);
        reader.optional(rfrom);
        reader.optional(rlen);
        if(reader.end_overload())
          return (Value) std_string_replace_slice(text, from, nullopt, rep, rfrom, rlen);

        reader.load_state(0);
        reader.optional(len);
        reader.required(rep);
        reader.optional(rfrom);
        reader.optional(rlen);
        if(reader.end_overload())
          return (Value) std_string_replace_slice(text, from, len, rep, rfrom, rlen);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"compare",
      ASTERIA_BINDING(
        "std.string.compare", "text1, text2, [length]",
        Argument_Reader&& reader)
      {
        V_string text1;
        V_string text2;
        optV_integer len;

        reader.start_overload();
        reader.required(text1);
        reader.required(text2);
        reader.optional(len);
        if(reader.end_overload())
          return (Value) std_string_compare(text1, text2, len);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"starts_with",
      ASTERIA_BINDING(
        "std.string.starts_with", "text, prefix",
        Argument_Reader&& reader)
      {
        V_string text;
        V_string prfx;

        reader.start_overload();
        reader.required(text);
        reader.required(prfx);
        if(reader.end_overload())
          return (Value) std_string_starts_with(text, prfx);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"ends_with",
      ASTERIA_BINDING(
        "std.string.ends_with", "text, suffix",
        Argument_Reader&& reader)
      {
        V_string text;
        V_string sufx;

        reader.start_overload();
        reader.required(text);
        reader.required(sufx);
        if(reader.end_overload())
          return (Value) std_string_ends_with(text, sufx);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"find",
      ASTERIA_BINDING(
        "std.string.find", "text, [from, [length]], pattern",
        Argument_Reader&& reader)
      {
        V_string text, patt;
        optV_integer from, len;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(patt);
        if(reader.end_overload())
          return (Value) std_string_find(text, 0, nullopt, patt);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(patt);
        if(reader.end_overload())
          return (Value) std_string_find(text, from, nullopt, patt);

        reader.load_state(1);
        reader.optional(len);
        reader.required(patt);
        if(reader.end_overload())
          return (Value) std_string_find(text, from, len, patt);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"rfind",
      ASTERIA_BINDING(
        "std.string.rfind", "text, [from, [length]], pattern",
        Argument_Reader&& reader)
      {
        V_string text, patt;
        optV_integer from, len;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(patt);
        if(reader.end_overload())
          return (Value) std_string_rfind(text, 0, nullopt, patt);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(0);
        reader.required(patt);
        if(reader.end_overload())
          return (Value) std_string_rfind(text, from, nullopt, patt);

        reader.load_state(0);
        reader.optional(len);
        reader.required(patt);
        if(reader.end_overload())
          return (Value) std_string_rfind(text, from, len, patt);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"replace",
      ASTERIA_BINDING(
        "std.string.replace", "text, [from, [length]], pattern, replacement",
        Argument_Reader&& reader)
      {
        V_string text, patt, rep;
        optV_integer from, len;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(patt);
        reader.required(rep);
        if(reader.end_overload())
          return (Value) std_string_replace(text, 0, nullopt, patt, rep);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(patt);
        reader.required(rep);
        if(reader.end_overload())
          return (Value) std_string_replace(text, from, nullopt, patt, rep);

        reader.load_state(1);
        reader.optional(len);
        reader.required(patt);
        reader.required(rep);
        if(reader.end_overload())
          return (Value) std_string_replace(text, from, len, patt, rep);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"find_any_of",
      ASTERIA_BINDING(
        "std.string.find_any_of", "text, [from, [length]], accept",
        Argument_Reader&& reader)
      {
        V_string text;
        optV_integer from, len;
        V_string acc;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(acc);
        if(reader.end_overload())
          return (Value) std_string_find_any_of(text, 0, nullopt, acc);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(acc);
        if(reader.end_overload())
          return (Value) std_string_find_any_of(text, from, nullopt, acc);

        reader.load_state(1);
        reader.optional(len);
        reader.required(acc);
        if(reader.end_overload())
          return (Value) std_string_find_any_of(text, from, len, acc);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"rfind_any_of",
      ASTERIA_BINDING(
        "std.string.rfind_any_of", "text, [from, [length]], accept",
        Argument_Reader&& reader)
      {
        V_string text;
        optV_integer from, len;
        V_string acc;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(acc);
        if(reader.end_overload())
          return (Value) std_string_rfind_any_of(text, 0, nullopt, acc);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(acc);
        if(reader.end_overload())
          return (Value) std_string_rfind_any_of(text, from, nullopt, acc);

        reader.load_state(1);
        reader.optional(len);
        reader.required(acc);
        if(reader.end_overload())
          return (Value) std_string_rfind_any_of(text, from, len, acc);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"find_not_of",
      ASTERIA_BINDING(
        "std.string.find_not_of", "text, [from, [length]], reject",
        Argument_Reader&& reader)
      {
        V_string text;
        optV_integer from, len;
        V_string rej;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(rej);
        if(reader.end_overload())
          return (Value) std_string_find_not_of(text, 0, nullopt, rej);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(rej);
        if(reader.end_overload())
          return (Value) std_string_find_not_of(text, from, nullopt, rej);

        reader.load_state(1);
        reader.optional(len);
        reader.required(rej);
        if(reader.end_overload())
          return (Value) std_string_find_not_of(text, from, len, rej);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"rfind_not_of",
      ASTERIA_BINDING(
        "std.string.rfind_not_of", "text, [from, [length]], reject",
        Argument_Reader&& reader)
      {
        V_string text;
        optV_integer from, len;
        V_string rej;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(rej);
        if(reader.end_overload())
          return (Value) std_string_rfind_not_of(text, 0, nullopt, rej);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(rej);
        if(reader.end_overload())
          return (Value) std_string_rfind_not_of(text, from, nullopt, rej);

        reader.load_state(1);
        reader.optional(len);
        reader.required(rej);
        if(reader.end_overload())
          return (Value) std_string_rfind_not_of(text, from, len, rej);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"reverse",
      ASTERIA_BINDING(
        "std.string.reverse", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_string_reverse(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"trim",
      ASTERIA_BINDING(
        "std.string.trim", "text, [reject]",
        Argument_Reader&& reader)
      {
        V_string text;
        optV_string rej;

        reader.start_overload();
        reader.required(text);
        reader.optional(rej);
        if(reader.end_overload())
          return (Value) std_string_trim(text, rej);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"triml",
      ASTERIA_BINDING(
        "std.string.triml", "text, [reject]",
        Argument_Reader&& reader)
      {
        V_string text;
        optV_string rej;

        reader.start_overload();
        reader.required(text);
        reader.optional(rej);
        if(reader.end_overload())
          return (Value) std_string_triml(text, rej);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"trimr",
      ASTERIA_BINDING(
        "std.string.trimr", "text, [reject]",
        Argument_Reader&& reader)
      {
        V_string text;
        optV_string rej;

        reader.start_overload();
        reader.required(text);
        reader.optional(rej);
        if(reader.end_overload())
          return (Value) std_string_trimr(text, rej);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"padl",
      ASTERIA_BINDING(
        "std.string.padl", "text, length, [padding]",
        Argument_Reader&& reader)
      {
        V_string text;
        V_integer len;
        optV_string pad;

        reader.start_overload();
        reader.required(text);
        reader.required(len);
        reader.optional(pad);
        if(reader.end_overload())
          return (Value) std_string_padl(text, len, pad);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"padr",
      ASTERIA_BINDING(
        "std.string.padr", "text, length, [padding]",
        Argument_Reader&& reader)
      {
        V_string text;
        V_integer len;
        optV_string pad;

        reader.start_overload();
        reader.required(text);
        reader.required(len);
        reader.optional(pad);
        if(reader.end_overload())
          return (Value) std_string_padr(text, len, pad);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"to_upper",
      ASTERIA_BINDING(
        "std.string.to_upper", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_string_to_upper(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"to_lower",
      ASTERIA_BINDING(
        "std.string.to_lower", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_string_to_lower(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"translate",
      ASTERIA_BINDING(
        "std.string.translate", "text, inputs, [outputs]",
        Argument_Reader&& reader)
      {
        V_string text;
        V_string in;
        optV_string out;

        reader.start_overload();
        reader.required(text);
        reader.required(in);
        reader.optional(out);
        if(reader.end_overload())
          return (Value) std_string_translate(text, in, out);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"explode",
      ASTERIA_BINDING(
        "std.string.explode", "text, [delim, [limit]]",
        Argument_Reader&& reader)
      {
        V_string text;
        optV_string delim;
        optV_integer limit;

        reader.start_overload();
        reader.required(text);
        reader.optional(delim);
        reader.optional(limit);
        if(reader.end_overload())
          return (Value) std_string_explode(text, delim, limit);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"implode",
      ASTERIA_BINDING(
        "std.string.implode", "segments, [delim]",
        Argument_Reader&& reader)
      {
        V_array segs;
        optV_string delim;

        reader.start_overload();
        reader.required(segs);
        reader.optional(delim);
        if(reader.end_overload())
          return (Value) std_string_implode(segs, delim);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"hex_encode",
      ASTERIA_BINDING(
        "std.string.hex_encode", "data, [delim]",
        Argument_Reader&& reader)
      {
        V_string data;
        optV_string delim;
        optV_boolean lowc;

        reader.start_overload();
        reader.required(data);
        reader.optional(delim);
        if(reader.end_overload())
          return (Value) std_string_hex_encode(data, delim);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"hex_decode",
      ASTERIA_BINDING(
        "std.string.hex_decode", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_string_hex_decode(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"base32_encode",
      ASTERIA_BINDING(
        "std.string.base32_encode", "data",
        Argument_Reader&& reader)
      {
        V_string data;
        optV_boolean lowc;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_string_base32_encode(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"base32_decode",
      ASTERIA_BINDING(
        "std.string.base32_decode", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_string_base32_decode(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"base64_encode",
      ASTERIA_BINDING(
        "std.string.base64_encode", "data",
        Argument_Reader&& reader)
      {
        V_string data;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_string_base64_encode(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"base64_decode",
      ASTERIA_BINDING(
        "std.string.base64_decode", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_string_base64_decode(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"url_encode",
      ASTERIA_BINDING(
        "std.string.url_encode", "data",
        Argument_Reader&& reader)
      {
        V_string data;
        optV_boolean lowc;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_string_url_encode(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"url_decode",
      ASTERIA_BINDING(
        "std.string.url_decode", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_string_url_decode(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"url_query_encode",
      ASTERIA_BINDING(
        "std.string.url_query_encode", "data",
        Argument_Reader&& reader)
      {
        V_string data;
        optV_boolean lowc;

        reader.start_overload();
        reader.required(data);
        if(reader.end_overload())
          return (Value) std_string_url_query_encode(data);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"url_query_decode",
      ASTERIA_BINDING(
        "std.string.url_query_decode", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_string_url_query_decode(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"utf8_validate",
     ASTERIA_BINDING(
       "std.string.utf8_validate", "text",
       Argument_Reader&& reader)
     {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_string_utf8_validate(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"utf8_encode",
      ASTERIA_BINDING(
        "std.string.utf8_encode", "code_points, [permissive]",
        Argument_Reader&& reader)
      {
        V_integer cp;
        V_array cps;
        optV_boolean perm;

        reader.start_overload();
        reader.required(cp);
        reader.optional(perm);
        if(reader.end_overload())
          return (Value) std_string_utf8_encode(cp, perm);

        reader.start_overload();
        reader.required(cps);
        reader.optional(perm);
        if(reader.end_overload())
          return (Value) std_string_utf8_encode(cps, perm);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"utf8_decode",
      ASTERIA_BINDING(
        "std.string.utf8_decode", "text, [permissive]",
        Argument_Reader&& reader)
      {
        V_string text;
        optV_boolean perm;

        reader.start_overload();
        reader.required(text);
        reader.optional(perm);
        if(reader.end_overload())
          return (Value) std_string_utf8_decode(text, perm);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"format",
      ASTERIA_BINDING(
        "std.string.format", "templ, ...",
        Argument_Reader&& reader)
      {
        V_string templ;
        cow_vector<Value> args;

        reader.start_overload();
        reader.required(templ);
        if(reader.end_overload(args))
          return (Value) std_string_format(templ, args);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"PCRE",
      ASTERIA_BINDING(
        "std.string.PCRE", "pattern, [options]",
        Argument_Reader&& reader)
      {
        V_string patt;
        optV_array opts;

        reader.start_overload();
        reader.required(patt);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_PCRE(patt, opts);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pcre_find",
      ASTERIA_BINDING(
        "std.string.pcre_find", "text, [from, [length]], pattern, [options]",
        Argument_Reader&& reader)
      {
        V_string text, patt;
        optV_integer from, len;
        optV_array opts;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(patt);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_find(text, 0, nullopt, patt, opts);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(patt);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_find(text, from, nullopt, patt, opts);

        reader.load_state(1);
        reader.optional(len);
        reader.required(patt);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_find(text, from, len, patt, opts);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pcre_match",
      ASTERIA_BINDING(
        "std.string.pcre_match", "text, [from, [length]], pattern, [options]",
        Argument_Reader&& reader)
      {
        V_string text, patt;
        optV_integer from, len;
        optV_array opts;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(patt);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_match(text, 0, nullopt, patt, opts);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(patt);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_match(text, from, nullopt, patt, opts);

        reader.load_state(1);
        reader.optional(len);
        reader.required(patt);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_match(text, from, len, patt, opts);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pcre_named_match",
      ASTERIA_BINDING(
        "std.string.pcre_named_match", "text, [from, [length]], pattern, [options]",
        Argument_Reader&& reader)
      {
        V_string text, patt;
        optV_integer from, len;
        optV_array opts;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(patt);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_named_match(text, 0, nullopt, patt, opts);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(patt);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_named_match(text, from, nullopt, patt, opts);

        reader.load_state(1);
        reader.optional(len);
        reader.required(patt);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_named_match(text, from, len, patt, opts);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"pcre_replace",
      ASTERIA_BINDING(
        "std.string.pcre_replace", "text, [from, [length]], pattern, replacement, [options]",
        Argument_Reader&& reader)
      {
        V_string text, patt, rep;
        optV_integer from, len;
        optV_array opts;

        reader.start_overload();
        reader.required(text);
        reader.save_state(0);
        reader.required(patt);
        reader.required(rep);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_replace(text, 0, nullopt, patt, rep, opts);

        reader.load_state(0);
        reader.optional(from);
        reader.save_state(1);
        reader.required(patt);
        reader.required(rep);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_replace(text, from, nullopt, patt, rep, opts);

        reader.load_state(1);
        reader.optional(len);
        reader.required(patt);
        reader.required(rep);
        reader.optional(opts);
        if(reader.end_overload())
          return (Value) std_string_pcre_replace(text, from, len, patt, rep, opts);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(&"iconv",
      ASTERIA_BINDING(
        "std.string.iconv", "to_encoding, text, [from_encoding]",
        Argument_Reader&& reader)
      {
        V_string to_encoding, text;
        optV_string from_encoding;

        reader.start_overload();
        reader.required(to_encoding);
        reader.required(text);
        reader.optional(from_encoding);
        if(reader.end_overload())
          return (Value) std_string_iconv(to_encoding, text, from_encoding);

        reader.throw_no_matching_function_call();
      });

    if(version >= api_version_0002_0000)
      result.insert_or_assign(&"visual_length",
        ASTERIA_BINDING(
          "visual_length", "text",
          Argument_Reader&& reader)
        {
          V_string text;

          reader.start_overload();
          reader.required(text);
          if(reader.end_overload())
            return (Value) std_string_visual_width(text);

          reader.throw_no_matching_function_call();
        });
  }

}  // namespace asteria
