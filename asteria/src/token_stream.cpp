// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "parser_error.hpp"
#include "utilities.hpp"
#include <algorithm>

namespace Asteria {

Token_stream::~Token_stream()
  {
  }

namespace {

  class Source_reader
    {
    private:
      const std::reference_wrapper<std::istream> m_strm;
      const String m_file;

      String m_str;
      Uint32 m_line;
      Size m_offset;

    public:
      Source_reader(std::istream &xstrm, String xfile)
        : m_strm(xstrm), m_file(std::move(xfile)),
          m_str(), m_line(0), m_offset(0)
        {
          // Check whether the stream can be read from.
          // For example, we shall fail here if an `std::ifstream` was constructed with a non-existent path.
          if(this->m_strm.get().fail()) {
            throw Parser_error(0, 0, 0, Parser_error::code_istream_open_failure);
          }
        }

      Source_reader(const Source_reader &) = delete;
      Source_reader & operator=(const Source_reader &) = delete;

    public:
      std::istream & stream() const noexcept
        {
          return this->m_strm;
        }
      const String & file() const noexcept
        {
          return this->m_file;
        }

      Uint32 line() const noexcept
        {
          return this->m_line;
        }
      bool advance_line()
        {
          // Call `getline()` via ADL.
          getline(this->m_strm.get(), this->m_str);
          // Check `bad()` before `fail()` because the latter checks for both `badbit` and `failbit`.
          if(this->m_strm.get().bad()) {
            throw Parser_error(this->m_line, this->m_offset, 0, Parser_error::code_istream_badbit_set);
          }
          if(this->m_strm.get().fail()) {
            return false;
          }
          // A line has been read successfully.
          ++(this->m_line);
          this->m_offset = 0;
          ASTERIA_DEBUG_LOG("Read line ", std::setw(4), this->m_line, ": ", this->m_str);
          return true;
        }
      Size offset() const noexcept
        {
          return this->m_offset;
        }
      const char * data_avail() const noexcept
        {
          return this->m_str.data() + this->m_offset;
        }
      Size size_avail() const noexcept
        {
          return this->m_str.size() - this->m_offset;
        }
      char peek(Size add = 0) const noexcept
        {
          if(add > this->size_avail()) {
            return 0;
          }
          return this->data_avail()[add];
        }
      void consume(Size add)
        {
          if(add > this->size_avail()) {
            ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to seek past the end of the current line.");
          }
          this->m_offset += add;
        }
      void rewind(Size xoffset = 0) noexcept
        {
          this->m_offset = xoffset;
        }
    };

  inline Parser_error do_make_parser_error(const Source_reader &reader, Size length, Parser_error::Code code)
    {
      return Parser_error(reader.line(), reader.offset(), length, code);
    }

  class Tack
    {
    private:
      Uint32 m_line;
      Size m_offset;
      Size m_length;

    public:
      constexpr Tack() noexcept
        : m_line(0), m_offset(0), m_length(0)
        {
        }

    public:
      constexpr Uint32 line() const noexcept
        {
          return this->m_line;
        }
      constexpr Size offset() const noexcept
        {
          return this->m_offset;
        }
      constexpr Size length() const noexcept
        {
          return this->m_length;
        }
      explicit constexpr operator bool () const noexcept
        {
          return this->m_line != 0;
        }
      Tack & set(const Source_reader &reader, Size xlength) noexcept
        {
          this->m_line = reader.line();
          this->m_offset = reader.offset();
          this->m_length = xlength;
          return *this;
        }
      Tack & clear() noexcept
        {
          this->m_line = 0;
          return *this;
        }
    };

  char32_t do_get_utf8_code_point(Source_reader &reader)
    {
      // Read the first byte.
      char32_t code_point = reader.peek() & 0xFF;
      if(code_point < 0x80) {
        // This sequence contains only one byte.
        reader.consume(1);
        return code_point;
      }
      if(code_point < 0xC0) {
        // This is not a leading character.
        throw do_make_parser_error(reader, 1, Parser_error::code_utf8_sequence_invalid);
      }
      if(code_point >= 0xF8) {
        // If this leading character were valid, it would start a sequence of five bytes or more.
        throw do_make_parser_error(reader, 1, Parser_error::code_utf8_sequence_invalid);
      }
      // Calculate the number of bytes in this code point.
      const auto u8len = static_cast<unsigned>(2 + (code_point >= 0xE0) + (code_point >= 0xF0));
      ROCKET_ASSERT(u8len >= 2);
      ROCKET_ASSERT(u8len <= 4);
      // Unset bits that are not part of the payload.
      code_point &= static_cast<unsigned char>(0xFF >> u8len);
      // Accumulate trailing code units.
      if(u8len > reader.size_avail()) {
        // No enough characters have been provided.
        throw do_make_parser_error(reader, reader.size_avail(), Parser_error::code_utf8_sequence_incomplete);
      }
      for(unsigned i = 1; i < u8len; ++i) {
        char32_t next = reader.peek(i) & 0xFF;
        if((next < 0x80) || (0xC0 <= next)) {
          // This trailing character is not valid.
          throw do_make_parser_error(reader, i + 1, Parser_error::code_utf8_sequence_invalid);
        }
        code_point = (code_point << 6) | (next & 0x3F);
      }
      if((0xD800 <= code_point) && (code_point < 0xE000)) {
        // Surrogates are not allowed.
        throw do_make_parser_error(reader, u8len, Parser_error::code_utf_code_point_invalid);
      }
      if(code_point >= 0x110000) {
        // Code point value is too large.
        throw do_make_parser_error(reader, u8len, Parser_error::code_utf_code_point_invalid);
      }
      // Re-encode it and check for overlong sequences.
      const auto minlen = static_cast<unsigned>(1 + (code_point >= 0x80) + (code_point >= 0x800) + (code_point >= 0x10000));
      if(minlen != u8len) {
        // Overlong sequences are not allowed.
        throw do_make_parser_error(reader, u8len, Parser_error::code_utf8_sequence_invalid);
      }
      reader.consume(u8len);
      return code_point;
    }

  template<typename TokenT>
    void do_push_token(Vector<Token> &seq_out, Source_reader &reader_io, Size length, TokenT &&token_c)
      {
        seq_out.emplace_back(reader_io.file(), reader_io.line(), reader_io.offset(), length, std::forward<TokenT>(token_c));
        reader_io.consume(length);
      }

  struct Prefix_comparator
    {
      template<typename ElementT>
        bool operator()(const ElementT &lhs, const ElementT &rhs) const noexcept
          {
            return std::char_traits<char>::compare(lhs.first, rhs.first, sizeof(lhs.first)) < 0;
          }
      template<typename ElementT>
        bool operator()(char lhs, const ElementT &rhs) const noexcept
          {
            return std::char_traits<char>::lt(lhs, rhs.first[0]);
          }
      template<typename ElementT>
        bool operator()(const ElementT &lhs, char rhs) const noexcept
          {
            return std::char_traits<char>::lt(lhs.first[0], rhs);
          }
    };

  bool do_accept_identifier_or_keyword(Vector<Token> &seq_out, Source_reader &reader_io)
    {
      // identifier ::=
      //   PCRE([A-Za-z_][A-Za-z_0-9]*)
      static constexpr char s_name_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789";
      const auto bptr = reader_io.data_avail();
      if(std::char_traits<char>::find(s_name_chars, 53, bptr[0]) == nullptr) {
        return false;
      }
      // Get an identifier.
      const auto eptr = bptr + reader_io.size_avail();
      auto tptr = std::find_if_not(bptr, eptr, [&](char ch) { return std::char_traits<char>::find(s_name_chars, 63, ch); });
      const auto tlen = static_cast<Size>(tptr - bptr);
      // Check whether this identifier matches a keyword.
      struct Keyword_element
        {
          char first[12];
          Token::Keyword second;
          long : 0;
        }
      static constexpr s_keywords[] =
        {
          { "break",     Token::keyword_break     },
          { "case",      Token::keyword_case      },
          { "catch",     Token::keyword_catch     },
          { "const",     Token::keyword_const     },
          { "continue",  Token::keyword_continue  },
          { "default",   Token::keyword_default   },
          { "defer",     Token::keyword_defer     },
          { "do",        Token::keyword_do        },
          { "each",      Token::keyword_each      },
          { "else",      Token::keyword_else      },
          { "false",     Token::keyword_false     },
          { "for",       Token::keyword_for       },
          { "func",      Token::keyword_func      },
          { "if",        Token::keyword_if        },
          { "infinity",  Token::keyword_infinity  },
          { "lengthof",  Token::keyword_lengthof     },
          { "nan",       Token::keyword_nan       },
          { "null",      Token::keyword_null      },
          { "return",    Token::keyword_return    },
          { "switch",    Token::keyword_switch    },
          { "this",      Token::keyword_this      },
          { "throw",     Token::keyword_throw     },
          { "true",      Token::keyword_true      },
          { "try",       Token::keyword_try       },
          { "unset",     Token::keyword_unset     },
          { "var",       Token::keyword_var       },
          { "while",     Token::keyword_while     },
        };
      ROCKET_ASSERT(std::is_sorted(std::begin(s_keywords), std::end(s_keywords), Prefix_comparator()));
      auto range = std::equal_range(std::begin(s_keywords), std::end(s_keywords), bptr[0], Prefix_comparator());
      for(;;) {
        if(range.first == range.second) {
          // No matching keyword has been found so far.
          Token::S_identifier token_c = { String(bptr, tlen) };
          do_push_token(seq_out, reader_io, tlen, std::move(token_c));
          return true;
        }
        const auto &cur = range.first[0];
        if((std::char_traits<char>::length(cur.first) == tlen) && (std::char_traits<char>::compare(bptr, cur.first, tlen) == 0)) {
          // A keyword has been found.
          Token::S_keyword token_c = { cur.second };
          do_push_token(seq_out, reader_io, tlen, std::move(token_c));
          return true;
        }
        ++(range.first);
      }
    }

  bool do_accept_punctuator(Vector<Token> &seq_out, Source_reader &reader_io)
    {
      static constexpr char s_punct_chars[] = "!%&()*+,-./:;<=>?[]^{|}~";
      const auto bptr = reader_io.data_avail();
      if(std::char_traits<char>::find(s_punct_chars, std::char_traits<char>::length(s_punct_chars), bptr[0]) == nullptr) {
        return false;
      }
      // Get a punctuator.
      struct Punctuator_element
        {
          char first[6];
          Token::Punctuator second;
          long : 0;
        }
      static constexpr s_punctuators[] =
        {
          { "!",     Token::punctuator_notl        },
          { "!=",    Token::punctuator_cmp_ne      },
          { "%",     Token::punctuator_mod         },
          { "%=",    Token::punctuator_mod_eq      },
          { "&",     Token::punctuator_andb        },
          { "&&",    Token::punctuator_andl        },
          { "&&=",   Token::punctuator_andl_eq     },
          { "&=",    Token::punctuator_andb_eq     },
          { "(",     Token::punctuator_parenth_op  },
          { ")",     Token::punctuator_parenth_cl  },
          { "*",     Token::punctuator_mul         },
          { "*=",    Token::punctuator_mul_eq      },
          { "+",     Token::punctuator_add         },
          { "++",    Token::punctuator_inc         },
          { "+=",    Token::punctuator_add_eq      },
          { ",",     Token::punctuator_comma       },
          { "-",     Token::punctuator_sub         },
          { "--",    Token::punctuator_dec         },
          { "-=",    Token::punctuator_sub_eq      },
          { ".",     Token::punctuator_dot         },
          { "...",   Token::punctuator_ellipsis    },
          { "/",     Token::punctuator_div         },
          { "/=",    Token::punctuator_div_eq      },
          { ":",     Token::punctuator_colon       },
          { ";",     Token::punctuator_semicol     },
          { "<",     Token::punctuator_cmp_lt      },
          { "<<",    Token::punctuator_sla         },
          { "<<<",   Token::punctuator_sll         },
          { "<<<=",  Token::punctuator_sll_eq      },
          { "<<=",   Token::punctuator_sla_eq      },
          { "<=",    Token::punctuator_cmp_lte     },
          { "<=>",   Token::punctuator_spaceship   },
          { "=",     Token::punctuator_assign      },
          { "==",    Token::punctuator_cmp_eq      },
          { ">",     Token::punctuator_cmp_gt      },
          { ">=",    Token::punctuator_cmp_gte     },
          { ">>",    Token::punctuator_sra         },
          { ">>=",   Token::punctuator_sra_eq      },
          { ">>>",   Token::punctuator_srl         },
          { ">>>=",  Token::punctuator_srl_eq      },
          { "?",     Token::punctuator_quest       },
          { "?=",    Token::punctuator_quest_eq    },
          { "?\?",   Token::punctuator_coales      },
          { "?\?=",  Token::punctuator_coales_eq   },
          { "[",     Token::punctuator_bracket_op  },
          { "]",     Token::punctuator_bracket_cl  },
          { "^",     Token::punctuator_xorb        },
          { "^=",    Token::punctuator_xorb_eq     },
          { "{",     Token::punctuator_brace_op    },
          { "|",     Token::punctuator_orb         },
          { "|=",    Token::punctuator_orb_eq      },
          { "||",    Token::punctuator_orl         },
          { "||=",   Token::punctuator_orl_eq      },
          { "}",     Token::punctuator_brace_cl    },
          { "~",     Token::punctuator_notb        },
        };
      ROCKET_ASSERT(std::is_sorted(std::begin(s_punctuators), std::end(s_punctuators), Prefix_comparator()));
      // For two elements X and Y, if X is in front of Y, then X is potential a prefix of Y.
      // Traverse the range backwards to prevent premature matches, as a token is defined to be the longest valid character sequence.
      auto range = std::equal_range(std::begin(s_punctuators), std::end(s_punctuators), bptr[0], Prefix_comparator());
      for(;;) {
        if(range.first == range.second) {
          // No matching punctuator has been found so far.
          // This is caused by a character in `punct_chars` that does not exist in the table above.
          ASTERIA_TERMINATE("The punctuator `", bptr[0], "` is unhandled.");
        }
        const auto &cur = range.second[-1];
        const auto tlen = std::char_traits<char>::length(cur.first);
        if((tlen <= reader_io.size_avail()) && (std::char_traits<char>::compare(bptr, cur.first, tlen) == 0)) {
          // A punctuator has been found.
          Token::S_punctuator token_c = { cur.second };
          do_push_token(seq_out, reader_io, tlen, std::move(token_c));
          return true;
        }
        --(range.second);
      }
    }

  bool do_accept_string_literal(Vector<Token> &seq_out, Source_reader &reader_io)
    {
      // string-literal ::=
      //   PCRE("([^\\]|(\\([abfnrtveZ0'"?\\]|(x[0-9A-Fa-f]{2})|(u[0-9A-Fa-f]{4})|(U[0-9A-Fa-f]{6}))))*?")
      const auto bptr = reader_io.data_avail();
      if(bptr[0] != '\"') {
        return false;
      }
      // Get a string literal with regard to escape sequences.
      Size tlen = 1;
      String value;
      for(;;) {
        const auto qavail = reader_io.size_avail() - tlen;
        if(qavail == 0) {
          throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_string_literal_unclosed);
        }
        auto next = bptr[tlen];
        ++tlen;
        if(next == '\"') {
          // The end of this string is encountered. Finish.
          break;
        }
        if(next != '\\') {
          // This character does not start an escape sequence. Copy it as-is.
          value.push_back(next);
          continue;
        }
        // Translate this escape sequence.
        if(qavail < 2) {
          throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_escape_sequence_incomplete);
        }
        next = bptr[tlen];
        ++tlen;
        unsigned xcnt = 0;
        switch(next) {
          case '\'':
          case '\"':
          case '\\':
          case '?': {
            value.push_back(next);
            break;
          }
          case 'a': {
            value.push_back('\a');
            break;
          }
          case 'b': {
            value.push_back('\b');
            break;
          }
          case 'f': {
            value.push_back('\f');
            break;
          }
          case 'n': {
            value.push_back('\n');
            break;
          }
          case 'r': {
            value.push_back('\r');
            break;
          }
          case 't': {
            value.push_back('\t');
            break;
          }
          case 'v': {
            value.push_back('\v');
            break;
          }
          case '0': {
            value.push_back('\0');
            break;
          }
          case 'Z': {
            value.push_back('\x1A');
            break;
          }
          case 'e': {
            value.push_back('\x1B');
            break;
          }
          case 'U': {
            xcnt += 2;  // 6: "\U123456"
            // Fallthrough.
          case 'u':
            xcnt += 2;  // 4: "\u1234"
            // Fallthrough.
          case 'x':
            xcnt += 2;  // 2: "\x12"
            // Read hex digits.
            if(qavail < xcnt + 2) {
              throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_escape_sequence_incomplete);
            }
            char32_t code_point = 0;
            for(auto i = tlen; i < tlen + xcnt; ++i) {
              static constexpr char s_digits[] = "00112233445566778899AaBbCcDdEeFf";
              const auto dptr = std::char_traits<char>::find(s_digits, 32, bptr[i]);
              if(!dptr) {
                throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_escape_sequence_invalid_hex);
              }
              const auto dvalue = static_cast<char32_t>((dptr - s_digits) / 2);
              code_point = code_point * 16 + dvalue;
            }
            if((0xD800 <= code_point) && (code_point < 0xE000)) {
              // Surrogates are not allowed.
              throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_escape_utf_code_point_invalid);
            }
            if(code_point >= 0x110000) {
              // Code point value is too large.
              throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_escape_utf_code_point_invalid);
            }
            // Encode it.
            const auto encode_one = [&](unsigned shift, unsigned mask)
              {
                value.push_back(static_cast<char>((~mask << 1) | ((code_point >> shift) & mask)));
              };
            if(code_point < 0x80) {
              encode_one( 0, 0xFF);
              break;
            }
            if(code_point < 0x800) {
              encode_one( 6, 0x1F);
              encode_one( 0, 0x3F);
              break;
            }
            if(code_point < 0x10000) {
              encode_one(12, 0x0F);
              encode_one( 6, 0x3F);
              encode_one( 0, 0x3F);
              break;
            }
            encode_one(18, 0x07);
            encode_one(12, 0x3F);
            encode_one( 6, 0x3F);
            encode_one( 0, 0x3F);
            break;
          }
          default: {
            // Fail if this escape sequence cannot be recognized.
            throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_escape_sequence_unknown);
          }
        }
        tlen += xcnt;
      }
      Token::S_string_literal token_c = { std::move(value) };
      do_push_token(seq_out, reader_io, tlen, std::move(token_c));
      return true;
    }

  bool do_accept_noescape_string_literal(Vector<Token> &seq_out, Source_reader &reader_io)
    {
      // noescape-string-literal ::=
      //   PCRE('[^']*?')
      const auto bptr = reader_io.data_avail();
      if(bptr[0] != '\'') {
        return false;
      }
      // Escape sequences do not have special meanings inside single quotation marks.
      Size tlen = 1;
      String value;
      {
        auto tptr = std::char_traits<char>::find(bptr + 1, reader_io.size_avail() - 1, '\'');
        if(!tptr) {
          throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_string_literal_unclosed);
        }
        tptr += 1;
        value.append(bptr + 1, tptr - 1);
        tlen = static_cast<Size>(tptr - bptr);
      }
      Token::S_string_literal token_c = { std::move(value) };
      do_push_token(seq_out, reader_io, tlen, std::move(token_c));
      return true;
    }

  bool do_accept_numeric_literal(Vector<Token> &seq_out, Source_reader &reader_io)
    {
      // numeric-literal ::=
      //   ( binary-literal | decimal-literal | hexadecimal-literal ) exponent-suffix-opt
      // exponent-suffix-opt ::=
      //   decimal-exponent-suffix | binary-exponent-suffix | ""
      // binary-literal ::=
      //   PCRE(0[bB][01`]+(\.[01`]+)
      // decimal-literal ::=
      //   PCRE([0-9`]+(\.[0-9`]+))
      // hexadecimal-literal ::=
      //   PCRE(0[xX][0-9A-Fa-f`]+(\.[0-9A-Fa-f`]+))
      // decimal-exponent-suffix ::=
      //   PCRE([eE][-+]?[0-9`]+)
      // binary-exponent-suffix ::=
      //   PCRE([pP][-+]?[0-9`]+)
      static constexpr char s_digits[] = "00112233445566778899AaBbCcDdEeFf";
      const auto bptr = reader_io.data_avail();
      if(std::char_traits<char>::find(s_digits, 20, bptr[0]) == nullptr) {
        return false;
      }
      const auto eptr = bptr + reader_io.size_avail();
      // Get a numeric literal.
      // Declare everything that will be calculated later.
      // 0. The integral part is required. The fractional and exponent parts are optional.
      // 1. If `frac_begin` equals `int_end` then there is no fractional part.
      // 2. If `exp_begin` equals `frac_end` then there is no exponent part.
      unsigned radix = 10;
      Size int_begin = 0, int_end = 0;
      Size frac_begin = 0, frac_end = 0;
      unsigned exp_base = 0;
      bool exp_sign = false;
      Size exp_begin = 0, exp_end = 0;
      // Check for radix prefixes.
      if(bptr[int_begin] == '0') {
        auto next = bptr[int_begin + 1];
        switch(next) {
          case 'B':
          case 'b': {
            radix = 2;
            int_begin += 2;
            break;
          }
          case 'X':
          case 'x': {
            radix = 16;
            int_begin += 2;
            break;
          }
        }
      }
      // Look for the end of the integral part.
      auto tptr = std::find_if_not(bptr + int_begin, eptr, [&](char ch) { return (ch == '`') || std::char_traits<char>::find(s_digits, radix * 2, ch); });
      int_end = static_cast<Size>(tptr - bptr);
      if(int_end == int_begin) {
        throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_numeric_literal_incomplete);
      }
      // Look for the fractional part.
      frac_begin = int_end;
      frac_end = frac_begin;
      auto next = bptr[int_end];
      if(next == '.') {
        frac_begin += 1;
        tptr = std::find_if_not(bptr + frac_begin, eptr, [&](char ch) { return (ch == '`') || std::char_traits<char>::find(s_digits, radix * 2, ch); });
        frac_end = static_cast<Size>(tptr - bptr);
        if(frac_end == frac_begin) {
          throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_numeric_literal_incomplete);
        }
      }
      // Look for the exponent.
      exp_begin = frac_end;
      exp_end = exp_begin;
      next = bptr[frac_end];
      switch(next) {
        case 'E':
        case 'e': {
          exp_base = 10;
          exp_begin += 1;
          break;
        }
        case 'P':
        case 'p': {
          exp_base = 2;
          exp_begin += 1;
          break;
        }
      }
      if(exp_base != 0) {
        next = bptr[exp_begin];
        switch(next) {
          case '+': {
            exp_sign = false;
            exp_begin += 1;
            break;
          }
          case '-': {
            exp_sign = true;
            exp_begin += 1;
            break;
          }
        }
        tptr = std::find_if_not(bptr + exp_begin, eptr, [&](char ch) { return (ch == '`') || std::char_traits<char>::find(s_digits, 20, ch); });
        exp_end = static_cast<Size>(tptr - bptr);
        if(exp_end == exp_begin) {
          throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_numeric_literal_incomplete);
        }
      }
      // Disallow suffixes. Suffixes such as `ll`, `u` and `f` are used in C and C++ to specify the types of numeric literals.
      // Since we make no use of them, we just reserve them for further use for good.
      static constexpr char s_suffix_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789.";
      tptr = std::find_if_not(bptr + exp_end, eptr, [&](char ch) { return std::char_traits<char>::find(s_suffix_chars, 64, ch); });
      if(tptr != bptr + exp_end) {
        throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_numeric_literal_suffix_disallowed);
      }
      const auto tlen = exp_end;
      // Parse the exponent.
      int exp = 0;
      for(auto i = exp_begin; i != exp_end; ++i) {
        const auto dptr = std::char_traits<char>::find(s_digits, 20, bptr[i]);
        if(!dptr) {
          continue;
        }
        const auto dvalue = static_cast<int>((dptr - s_digits) / 2);
        const auto bound = (std::numeric_limits<int>::max() - dvalue) / 10;
        if(exp > bound) {
          throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_numeric_literal_exponent_overflow);
        }
        exp = exp * 10 + dvalue;
      }
      if(exp_sign) {
        exp = -exp;
      }
      // Is this literal an integral or floating-point number?
      if(frac_begin == int_end) {
        // Parse the literal as an integer.
        // Negative exponents are not allowed, even when the significant part is zero.
        if(exp < 0) {
          throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_integer_literal_exponent_negative);
        }
        // Parse the significant part.
        Uint64 value = 0;
        for(auto i = int_begin; i != int_end; ++i) {
          const auto dptr = std::char_traits<char>::find(s_digits, radix * 2, bptr[i]);
          if(!dptr) {
            continue;
          }
          const auto dvalue = static_cast<Uint64>((dptr - s_digits) / 2);
          const auto bound = (std::numeric_limits<Uint64>::max() - dvalue) / radix;
          if(value > bound) {
            throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_integer_literal_overflow);
          }
          value = value * radix + dvalue;
        }
        // Raise the significant part to the power of `exp`.
        if(value != 0) {
          for(int i = 0; i < exp; ++i) {
            const auto bound = std::numeric_limits<Uint64>::max() / exp_base;
            if(value > bound) {
              throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_integer_literal_overflow);
            }
            value *= exp_base;
          }
        }
        // Push an integer literal.
        Token::S_integer_literal token_c = { value };
        do_push_token(seq_out, reader_io, tlen, std::move(token_c));
        return true;
      }
      // Parse the literal as a floating-point number.
      // Parse the integral part.
      Xfloat value = 0;
      bool zero = true;
      for(auto i = int_begin; i != int_end; ++i) {
        const auto dptr = std::char_traits<char>::find(s_digits, radix * 2, bptr[i]);
        if(!dptr) {
          continue;
        }
        const auto dvalue = static_cast<int>((dptr - s_digits) / 2);
        value = value * static_cast<int>(radix) + dvalue;
        zero |= dvalue;
      }
      int vclass = std::fpclassify(value);
      if(vclass == FP_INFINITE) {
        throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_real_literal_overflow);
      }
      // Parse the fractional part.
      Xfloat frac = 0;
      for(auto i = frac_end - 1; i + 1 != frac_begin; --i) {
        const auto dptr = std::char_traits<char>::find(s_digits, radix * 2, bptr[i]);
        if(!dptr) {
          continue;
        }
        const auto dvalue = static_cast<int>((dptr - s_digits) / 2);
        frac = (frac + dvalue) / static_cast<int>(radix);
        zero |= dvalue;
      }
      value += frac;
      // Raise the significant part to the power of `exp`.
      if(exp_base == FLT_RADIX) {
        value = std::scalbn(value, exp);
      } else {
        value = value * std::pow(static_cast<int>(exp_base), exp);
      }
      vclass = std::fpclassify(value);
      if(vclass == FP_INFINITE) {
        throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_real_literal_overflow);
      }
      if((vclass == FP_ZERO) && !zero) {
        throw do_make_parser_error(reader_io, reader_io.size_avail(), Parser_error::code_real_literal_underflow);
      }
      // Push a floating-point literal.
      Token::S_real_literal token_c = { value };
      do_push_token(seq_out, reader_io, tlen, std::move(token_c));
      return true;
    }

}

Parser_error Token_stream::get_parser_error() const noexcept
  {
    switch(this->state()) {
      case state_empty: {
        return Parser_error(0, 0, 0, Parser_error::code_no_data_loaded);
      }
      case state_error: {
        return this->m_stor.as<Parser_error>();
      }
      case state_success: {
        return Parser_error(0, 0, 0, Parser_error::code_success);
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }

bool Token_stream::empty() const noexcept
  {
    switch(this->state()) {
      case state_empty: {
        return true;
      }
      case state_error: {
        return true;
      }
      case state_success: {
        return this->m_stor.as<Vector<Token>>().empty();
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }

bool Token_stream::load(std::istream &cstrm_io, const String &file)
  try {
    // This has to be done before anything else because of possibility of exceptions.
    this->m_stor.set(nullptr);
    // Store tokens parsed here in normal order.
    // We will have to reverse this sequence before storing it into `*this` if it is accepted.
    Vector<Token> seq;
    // Save the position of an unterminated block comment.
    Tack bcomm;
    // Read source code line by line.
    Source_reader reader(cstrm_io, file);
    while(reader.advance_line()) {
      // Discard the first line if it looks like a shebang.
      if((reader.line() == 1) && (reader.size_avail() >= 2) && (std::char_traits<char>::compare(reader.data_avail(), "#!", 2) == 0)) {
        continue;
      }
      /////////////////////////////////////////////////////////////////////////
      // Phase 1
      //   Ensure this line is a valid UTF-8 string.
      /////////////////////////////////////////////////////////////////////////
      while(reader.size_avail() != 0) {
        do_get_utf8_code_point(reader);
      }
      reader.rewind();
      /////////////////////////////////////////////////////////////////////////
      // Phase 2
      //   Break this line down into tokens.
      /////////////////////////////////////////////////////////////////////////
      while(reader.size_avail() != 0) {
        // Are we inside a block comment?
        if(bcomm) {
          // Search for the terminator of this block comment.
          static constexpr char s_bcomm_term[2] = { '*', '/' };
          const auto bptr = reader.data_avail();
          const auto eptr = bptr + reader.size_avail();
          auto tptr = std::search(bptr, eptr, s_bcomm_term, s_bcomm_term + 2);
          if(tptr == eptr) {
            // The block comment will not end in this line. Stop.
            reader.consume(reader.size_avail());
            break;
          }
          tptr += 2;
          // Finish this comment and resume from the end of it.
          bcomm.clear();
          reader.consume(static_cast<Size>(tptr - bptr));
          continue;
        }
        // Read a character.
        const auto head = reader.peek();
        if(std::char_traits<char>::find(" \t\v\f\r\n", 6, head) != nullptr) {
          // Skip a space.
          reader.consume(1);
          continue;
        }
        if(head == '/') {
          const auto next = reader.peek(1);
          if(next == '/') {
            // Start a line comment. Discard all remaining characters in this line.
            reader.consume(reader.size_avail());
            break;
          }
          if(next == '*') {
            // Start a block comment.
            bcomm.set(reader, 2);
            reader.consume(2);
            continue;
          }
        }
        bool token_got = do_accept_identifier_or_keyword(seq, reader) ||
                         do_accept_punctuator(seq, reader) ||
                         do_accept_string_literal(seq, reader) ||
                         do_accept_noescape_string_literal(seq, reader) ||
                         do_accept_numeric_literal(seq, reader);
        if(!token_got) {
          ASTERIA_DEBUG_LOG("Non-token character encountered in source code: ", reader.data_avail());
          throw do_make_parser_error(reader, 1, Parser_error::code_token_character_unrecognized);
        }
      }
      reader.rewind();
    }
    if(bcomm) {
      // A block comment may straddle multiple lines. We just mark the first line here.
      throw Parser_error(bcomm.line(), bcomm.offset(), bcomm.length(), Parser_error::code_block_comment_unclosed);
    }
    // Accept the result.
    std::reverse(seq.mut_begin(), seq.mut_end());
    this->m_stor.set(std::move(seq));
    return true;
  } catch(Parser_error &err) {  // Don't play this at home.
    ASTERIA_DEBUG_LOG("Parser error: line = ", err.line(), ", offset = ", err.offset(), ", length = ", err.length(),
                      ", code = ", err.code(), " (", Parser_error::get_code_description(err.code()), ")");
    this->m_stor.set(std::move(err));
    return false;
  }

void Token_stream::clear() noexcept
  {
    this->m_stor.set(nullptr);
  }

const Token * Token_stream::peek_opt() const noexcept
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
      case state_success: {
        const auto &alt = this->m_stor.as<Vector<Token>>();
        if(alt.empty()) {
          return nullptr;
        }
        return &(alt.back());
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }

Token Token_stream::shift()
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
      case state_success: {
        auto &alt = this->m_stor.as<Vector<Token>>();
        if(alt.empty()) {
          ASTERIA_THROW_RUNTIME_ERROR("There are no more tokens from this stream.");
        }
        auto token = std::move(alt.mut_back());
        alt.pop_back();
        return std::move(token);
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }

}
