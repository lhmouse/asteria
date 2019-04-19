// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "parser_error.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    class Line_Reader
      {
      private:
        std::reference_wrapper<std::streambuf> m_cbuf;
        Cow_String m_file;

        Cow_String m_str;
        std::uint32_t m_line;
        std::size_t m_offset;

      public:
        Line_Reader(std::streambuf& xcbuf, const Cow_String& xfile)
          : m_cbuf(xcbuf), m_file(xfile),
            m_str(), m_line(0), m_offset(0)
          {
          }

        Line_Reader(const Line_Reader&)
          = delete;
        Line_Reader& operator=(const Line_Reader&)
          = delete;

      public:
        std::streambuf& cbuf() const noexcept
          {
            return this->m_cbuf;
          }
        const Cow_String& file() const noexcept
          {
            return this->m_file;
          }

        std::uint32_t line() const noexcept
          {
            return this->m_line;
          }
        bool advance()
          {
            // Clear the current line buffer.
            this->m_str.clear();
            this->m_offset = 0;
            // Buffer a line.
            for(;;) {
              auto ich = this->m_cbuf.get().sbumpc();
              if(ich == std::char_traits<char>::eof()) {
                // Return `false` to indicate that there are no more data, when nothing has been read so far.
                if(this->m_str.empty()) {
                  return false;
                }
                break;
              }
              if(ich == '\n') {
                break;
              }
              this->m_str.push_back(static_cast<char>(ich));
            }
            // Increment the line number.
            this->m_line++;
            ASTERIA_DEBUG_LOG("Read line ", std::setw(4), this->m_line, ": ", this->m_str);
            return true;
          }
        std::size_t offset() const noexcept
          {
            return this->m_offset;
          }
        const char* data_avail() const noexcept
          {
            return this->m_str.data() + this->m_offset;
          }
        std::size_t size_avail() const noexcept
          {
            return this->m_str.size() - this->m_offset;
          }
        char peek(std::size_t add = 0) const noexcept
          {
            if(add > this->size_avail()) {
              return 0;
            }
            return this->data_avail()[add];
          }
        void consume(std::size_t add)
          {
            if(add > this->size_avail()) {
              ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to seek past the end of the current line.");
            }
            this->m_offset += add;
          }
        void rewind(std::size_t xoffset = 0) noexcept
          {
            this->m_offset = xoffset;
          }
      };

    inline Parser_Error do_make_parser_error(const Line_Reader& reader, std::size_t length, Parser_Error::Code code)
      {
        return Parser_Error(reader.line(), reader.offset(), length, code);
      }

    class Tack
      {
      private:
        std::uint32_t m_line;
        std::size_t m_offset;
        std::size_t m_length;

      public:
        constexpr Tack() noexcept
          : m_line(0), m_offset(0), m_length(0)
          {
          }

      public:
        constexpr std::uint32_t line() const noexcept
          {
            return this->m_line;
          }
        constexpr std::size_t offset() const noexcept
          {
            return this->m_offset;
          }
        constexpr std::size_t length() const noexcept
          {
            return this->m_length;
          }
        explicit constexpr operator bool () const noexcept
          {
            return this->m_line != 0;
          }
        Tack& set(const Line_Reader& reader, std::size_t xlength) noexcept
          {
            this->m_line = reader.line();
            this->m_offset = reader.offset();
            this->m_length = xlength;
            return *this;
          }
        Tack& clear() noexcept
          {
            this->m_line = 0;
            return *this;
          }
      };

    template<typename XtokenT> void do_push_token(Cow_Vector<Token>& seq, Line_Reader& reader, std::size_t length, XtokenT&& xtoken)
      {
        seq.emplace_back(reader.file(), reader.line(), reader.offset(), length, rocket::forward<XtokenT>(xtoken));
        reader.consume(length);
      }

    struct Prefix_Comparator
      {
        template<typename ElementT> bool operator()(const ElementT& lhs, const ElementT& rhs) const noexcept
          {
            return std::char_traits<char>::compare(lhs.first, rhs.first, sizeof(lhs.first)) < 0;
          }
        template<typename ElementT> bool operator()(char lhs, const ElementT& rhs) const noexcept
          {
            return std::char_traits<char>::lt(lhs, rhs.first[0]);
          }
        template<typename ElementT> bool operator()(const ElementT& lhs, char rhs) const noexcept
          {
            return std::char_traits<char>::lt(lhs.first[0], rhs);
          }
      };

    bool do_accept_punctuator(Cow_Vector<Token>& seq, Line_Reader& reader)
      {
        static constexpr char s_punct_chars[] = "!%&()*+,-./:;<=>?[]^{|}~";
        auto bptr = reader.data_avail();
        if(std::char_traits<char>::find(s_punct_chars, std::char_traits<char>::length(s_punct_chars), bptr[0]) == nullptr) {
          return false;
        }
        // Get a punctuator.
        struct Punctuator_Element
          {
            char first[6];
            Token::Punctuator second;
            std::uintptr_t : 0;
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
#ifdef ROCKET_DEBUG
        ROCKET_ASSERT(std::is_sorted(std::begin(s_punctuators), std::end(s_punctuators), Prefix_Comparator()));
#endif
        // For two elements X and Y, if X is in front of Y, then X is potential a prefix of Y.
        // Traverse the range backwards to prevent premature matches, as a token is defined to be the longest valid character sequence.
        auto range = std::equal_range(std::begin(s_punctuators), std::end(s_punctuators), bptr[0], Prefix_Comparator());
        for(;;) {
          if(range.first == range.second) {
            break;
          }
          const auto& cur = range.second[-1];
          auto tlen = std::char_traits<char>::length(cur.first);
          if((tlen <= reader.size_avail()) && (std::char_traits<char>::compare(bptr, cur.first, tlen) == 0)) {
            // A punctuator has been found.
            Token::S_punctuator xtoken = { cur.second };
            do_push_token(seq, reader, tlen, rocket::move(xtoken));
            return true;
          }
          range.second--;
        }
        // No matching punctuator has been found so far.
        // This is caused by a character in `punct_chars` that does not exist in the table above.
        ASTERIA_TERMINATE("The punctuator `", bptr[0], "` is unhandled.");
      }

    bool do_accept_string_literal(Cow_Vector<Token>& seq, Line_Reader& reader, char head, bool escapable)
      {
        // string-literal ::=
        //   PCRE("([^\\]|(\\([abfnrtveZ0'"?\\]|(x[0-9A-Fa-f]{2})|(u[0-9A-Fa-f]{4})|(U[0-9A-Fa-f]{6}))))*?")
        // noescape-string-literal ::=
        //   PCRE('[^']*?')
        auto bptr = reader.data_avail();
        if(bptr[0] != head) {
          return false;
        }
        // Get a string literal.
        std::size_t tlen = 1;
        Cow_String value;
        if(escapable) {
          // Translate escape sequences as needed.
          for(;;) {
            auto qavail = reader.size_avail() - tlen;
            if(qavail == 0) {
              throw do_make_parser_error(reader, reader.size_avail(), Parser_Error::code_string_literal_unclosed);
            }
            auto next = bptr[tlen];
            ++tlen;
            if(next == head) {
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
              throw do_make_parser_error(reader, reader.size_avail(), Parser_Error::code_escape_sequence_incomplete);
            }
            next = bptr[tlen];
            ++tlen;
            unsigned xcnt = 0;
            switch(next) {
            case '\'':
            case '\"':
            case '\\':
            case '?':
              {
                value.push_back(next);
                break;
              }
            case 'a':
              {
                value.push_back('\a');
                break;
              }
            case 'b':
              {
                value.push_back('\b');
                break;
              }
            case 'f':
              {
                value.push_back('\f');
                break;
              }
            case 'n':
              {
                value.push_back('\n');
                break;
              }
            case 'r':
              {
                value.push_back('\r');
                break;
              }
            case 't':
              {
                value.push_back('\t');
                break;
              }
            case 'v':
              {
                value.push_back('\v');
                break;
              }
            case '0':
              {
                value.push_back('\0');
                break;
              }
            case 'Z':
              {
                value.push_back('\x1A');
                break;
              }
            case 'e':
              {
                value.push_back('\x1B');
                break;
              }
            case 'U':
              {
                xcnt += 2;  // 6: "\U123456"
              }
              // Fallthrough.
            case 'u':
              {
                xcnt += 2;  // 4: "\u1234"
              }
              // Fallthrough.
            case 'x':
              {
                xcnt += 2;  // 2: "\x12"
                // Read hex digits.
                if(qavail < xcnt + 2) {
                  throw do_make_parser_error(reader, reader.size_avail(), Parser_Error::code_escape_sequence_incomplete);
                }
                char32_t cp = 0;
                for(auto i = tlen; i < tlen + xcnt; ++i) {
                  static constexpr char s_digits[] = "00112233445566778899AaBbCcDdEeFf";
                  auto dptr = std::char_traits<char>::find(s_digits, 32, bptr[i]);
                  if(!dptr) {
                    throw do_make_parser_error(reader, i + 1, Parser_Error::code_escape_sequence_invalid_hex);
                  }
                  auto dvalue = static_cast<char32_t>((dptr - s_digits) / 2);
                  cp = cp * 16 + dvalue;
                }
                if(next == 'x') {
                  // Write the character verbatim.
                  value.push_back(static_cast<char>(cp));
                  break;
                }
                // Write a Unicode code point.
                if(!utf8_encode(value, cp)) {
                  // Code point value is reserved or too large.
                  throw do_make_parser_error(reader, tlen + xcnt, Parser_Error::code_escape_utf_code_point_invalid);
                }
                break;
              }
            default:
              throw do_make_parser_error(reader, tlen, Parser_Error::code_escape_sequence_unknown);
            }
            tlen += xcnt;
          }
        } else {
          // Copy escape sequences verbatim.
          auto tptr = std::char_traits<char>::find(bptr + 1, reader.size_avail() - 1, head);
          if(!tptr) {
            throw do_make_parser_error(reader, reader.size_avail(), Parser_Error::code_string_literal_unclosed);
          }
          ++tptr;
          value.append(bptr + 1, tptr - 1);
          tlen = static_cast<std::size_t>(tptr - bptr);
        }
        Token::S_string_literal xtoken = { rocket::move(value) };
        do_push_token(seq, reader, tlen, rocket::move(xtoken));
        return true;
      }

    bool do_accept_identifier_or_keyword(Cow_Vector<Token>& seq, Line_Reader& reader, bool keyword_as_identifier)
      {
        // identifier ::=
        //   PCRE([A-Za-z_][A-Za-z_0-9]*)
        static constexpr char s_name_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789";
        auto bptr = reader.data_avail();
        if(std::char_traits<char>::find(s_name_chars, 53, bptr[0]) == nullptr) {
          return false;
        }
        // Get an identifier.
        auto eptr = bptr + reader.size_avail();
        auto tptr = std::find_if_not(bptr, eptr, [&](char ch) { return std::char_traits<char>::find(s_name_chars, 63, ch);  });
        auto tlen = static_cast<std::size_t>(tptr - bptr);
        if(!keyword_as_identifier) {
          // Check whether this identifier matches a keyword.
          struct Keyword_Element
            {
              char first[12];
              Token::Keyword second;
              std::uintptr_t : 0;
            }
          static constexpr s_keywords[] =
            {
              { "__abs",     Token::keyword_abs       },
              { "__ceil",    Token::keyword_ceil      },
              { "__floor",   Token::keyword_floor     },
              { "__iceil",   Token::keyword_iceil     },
              { "__ifloor",  Token::keyword_ifloor    },
              { "__iround",  Token::keyword_iround    },
              { "__isinf",   Token::keyword_isinf     },
              { "__isnan",   Token::keyword_isnan     },
              { "__itrunc",  Token::keyword_itrunc    },
              { "__round",   Token::keyword_round     },
              { "__sqrt",    Token::keyword_sqrt      },
              { "__trunc",   Token::keyword_trunc     },
              { "and",       Token::keyword_and       },
              { "assert",    Token::keyword_assert    },
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
              { "lengthof",  Token::keyword_lengthof  },
              { "nan",       Token::keyword_nan       },
              { "not",       Token::keyword_not       },
              { "null",      Token::keyword_null      },
              { "or",        Token::keyword_or        },
              { "return",    Token::keyword_return    },
              { "switch",    Token::keyword_switch    },
              { "this",      Token::keyword_this      },
              { "throw",     Token::keyword_throw     },
              { "true",      Token::keyword_true      },
              { "try",       Token::keyword_try       },
              { "typeof",    Token::keyword_typeof    },
              { "unset",     Token::keyword_unset     },
              { "var",       Token::keyword_var       },
              { "while",     Token::keyword_while     },
            };
#ifdef ROCKET_DEBUG
          ROCKET_ASSERT(std::is_sorted(std::begin(s_keywords), std::end(s_keywords), Prefix_Comparator()));
#endif
          auto range = std::equal_range(std::begin(s_keywords), std::end(s_keywords), bptr[0], Prefix_Comparator());
          for(;;) {
            if(range.first == range.second) {
              // No matching keyword has been found so far.
              break;
            }
            const auto& cur = range.first[0];
            if((std::char_traits<char>::length(cur.first) == tlen) && (std::char_traits<char>::compare(bptr, cur.first, tlen) == 0)) {
              // A keyword has been found.
              Token::S_keyword xtoken = { cur.second };
              do_push_token(seq, reader, tlen, rocket::move(xtoken));
              return true;
            }
            range.first++;
          }
        }
        Token::S_identifier xtoken = { Cow_String(bptr, tlen) };
        do_push_token(seq, reader, tlen, rocket::move(xtoken));
        return true;
      }

    Token* do_check_mergeability(Cow_Vector<Token>& seq, const Line_Reader& reader)
      {
        if(seq.empty()) {
          // No previuos token exists.
          return nullptr;
        }
        auto qstok = std::addressof(seq.mut_back());
        if(qstok->line() != reader.line()) {
          // Not on the same line.
          return nullptr;
        }
        if(qstok->offset() + qstok->length() != reader.offset()) {
          // Not contiguous.
          return nullptr;
        }
        if(!qstok->is_punctuator()) {
          // Only an immediate `+` or `-` can be merged.
          return nullptr;
        }
        if(rocket::is_none_of(qstok->as_punctuator(), { Token::punctuator_add, Token::punctuator_sub })) {
          // Only an immediate `+` or `-` can be merged.
          return nullptr;
        }
        if(seq.size() < 2) {
          // Mergeable.
          return qstok;
        }
        // Check whether the previous token may be an infix operator.
        const auto& pt = seq.rbegin()[1];
        if(pt.is_keyword()) {
          // Mergeable unless the keyword denotes a value or reference.
          bool mergeable = rocket::is_none_of(pt.as_keyword(), { Token::keyword_null, Token::keyword_true, Token::keyword_false,
                                                                 Token::keyword_nan, Token::keyword_infinity, Token::keyword_this });
          return mergeable ? qstok : nullptr;
        }
        if(pt.is_punctuator()) {
          // Mergeable unless the punctuator terminates an expression.
          bool mergeable = rocket::is_none_of(pt.as_punctuator(), { Token::punctuator_inc, Token::punctuator_dec,
                                                                    Token::punctuator_parenth_cl, Token::punctuator_bracket_cl, Token::punctuator_brace_cl });
          return mergeable ? qstok : nullptr;
        }
        // Not mergeable.
        return nullptr;
      }

    bool do_accept_numeric_literal(Cow_Vector<Token>& seq, Line_Reader& reader, bool integer_as_real)
      {
        // numeric-literal ::=
        //   ( binary-literal | decimal-literal | hexadecimal-literal ) exponent-suffix-opt
        // exponent-suffix-opt ::=
        //   decimal-exponent-suffix | binary-exponent-suffix | ""
        // binary-literal ::=
        //   PCRE(0[bB][01`]+(\.[01`]+))
        // decimal-literal ::=
        //   PCRE([0-9`]+(\.[0-9`]+))
        // hexadecimal-literal ::=
        //   PCRE(0[xX][0-9A-Fa-f`]+(\.[0-9A-Fa-f`]+))
        // decimal-exponent-suffix ::=
        //   PCRE([eE][-+]?[0-9`]+)
        // binary-exponent-suffix ::=
        //   PCRE([pP][-+]?[0-9`]+)
        static constexpr char s_digits[] = "00112233445566778899AaBbCcDdEeFf";
        auto bptr = reader.data_avail();
        if(std::char_traits<char>::find(s_digits, 20, bptr[0]) == nullptr) {
          return false;
        }
        auto eptr = bptr + reader.size_avail();
        // Get a numeric literal.
        // Declare everything that will be calculated later.
        // 0. The integral part is required. The fractional and exponent parts are optional.
        // 1. If `frac_begin` equals `int_end` then there is no fractional part.
        // 2. If `exp_begin` equals `frac_end` then there is no exponent part.
        int radix = 10;
        std::size_t int_begin = 0, int_end = 0;
        std::size_t frac_begin = 0, frac_end = 0;
        int exp_base = 0;
        bool exp_sign = false;
        std::size_t exp_begin = 0, exp_end = 0;
        // Check for radix prefixes.
        if(bptr[int_begin] == '0') {
          auto next = bptr[int_begin + 1];
          switch(next) {
          case 'B':
          case 'b':
            radix = 2;
            int_begin += 2;
            break;
          case 'X':
          case 'x':
            radix = 16;
            int_begin += 2;
            break;
          }
        }
        auto max_digits = static_cast<std::size_t>(radix * 2);
        // Look for the end of the integral part.
        auto tptr = std::find_if_not(bptr + int_begin, eptr, [&](char ch) { return (ch == '`') || std::char_traits<char>::find(s_digits, max_digits, ch);  });
        int_end = static_cast<std::size_t>(tptr - bptr);
        if(int_end == int_begin) {
          throw do_make_parser_error(reader, int_end, Parser_Error::code_numeric_literal_incomplete);
        }
        // Look for the fractional part.
        frac_begin = int_end;
        frac_end = frac_begin;
        auto next = bptr[int_end];
        if(next == '.') {
          ++frac_begin;
          tptr = std::find_if_not(bptr + frac_begin, eptr, [&](char ch) { return (ch == '`') || std::char_traits<char>::find(s_digits, max_digits, ch);  });
          frac_end = static_cast<std::size_t>(tptr - bptr);
          if(frac_end == frac_begin) {
            throw do_make_parser_error(reader, frac_end, Parser_Error::code_numeric_literal_incomplete);
          }
        }
        // Look for the exponent.
        exp_begin = frac_end;
        exp_end = exp_begin;
        next = bptr[frac_end];
        switch(next) {
        case 'E':
        case 'e':
          exp_base = 10;
          ++exp_begin;
          break;
        case 'P':
        case 'p':
          exp_base = 2;
          ++exp_begin;
          break;
        }
        if(exp_base != 0) {
          next = bptr[exp_begin];
          switch(next) {
          case '+':
            exp_sign = false;
            ++exp_begin;
            break;
          case '-':
            exp_sign = true;
            ++exp_begin;
            break;
          }
          tptr = std::find_if_not(bptr + exp_begin, eptr, [&](char ch) { return (ch == '`') || std::char_traits<char>::find(s_digits, 20, ch);  });
          exp_end = static_cast<std::size_t>(tptr - bptr);
          if(exp_end == exp_begin) {
            throw do_make_parser_error(reader, exp_end, Parser_Error::code_numeric_literal_incomplete);
          }
        }
        // Disallow suffixes. Suffixes such as `ll`, `u` and `f` are used in C and C++ to specify the types of numeric literals.
        // Since we make no use of them, we just reserve them for further use for good.
        static constexpr char s_suffix_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789.";
        tptr = std::find_if_not(bptr + exp_end, eptr, [&](char ch) { return std::char_traits<char>::find(s_suffix_chars, 64, ch);  });
        auto tlen = static_cast<std::size_t>(tptr - bptr);
        if(tlen != exp_end) {
          throw do_make_parser_error(reader, tlen, Parser_Error::code_numeric_literal_suffix_disallowed);
        }
        // Parse the exponent.
        int exp = 0;
        for(auto i = exp_begin; i != exp_end; ++i) {
          auto dptr = std::char_traits<char>::find(s_digits, 20, bptr[i]);
          if(!dptr) {
            continue;
          }
          auto dvalue = static_cast<int>((dptr - s_digits) / 2);
          int bound = (INT_MAX - dvalue) / 10;
          if(exp > bound) {
            throw do_make_parser_error(reader, tlen, Parser_Error::code_numeric_literal_exponent_overflow);
          }
          exp = exp * 10 + dvalue;
        }
        if(exp_sign) {
          exp = -exp;
        }
        // Is this literal an integral or floating-point number?
        if(!integer_as_real && (frac_begin == int_end)) {
          // Parse the literal as an integer.
          // Negative exponents are not allowed, even when the significant part is zero.
          if(exp < 0) {
            throw do_make_parser_error(reader, tlen, Parser_Error::code_integer_literal_exponent_negative);
          }
          // Parse the significant part.
          std::uint64_t value = 0;
          for(auto i = int_begin; i != int_end; ++i) {
            auto dptr = std::char_traits<char>::find(s_digits, max_digits, bptr[i]);
            if(!dptr) {
              continue;
            }
            auto dvalue = static_cast<std::uint64_t>((dptr - s_digits) / 2);
            std::uint64_t bound = ((UINT64_C(1) << 63) - dvalue) / static_cast<unsigned>(radix);
            if(value > bound) {
              throw do_make_parser_error(reader, tlen, Parser_Error::code_integer_literal_overflow);
            }
            value = value * static_cast<unsigned>(radix) + dvalue;
          }
          // Raise the significant part to the power of `exp`.
          if(value != 0) {
            for(int i = 0; i < exp; ++i) {
              std::uint64_t bound = (UINT64_C(1) << 63) / static_cast<unsigned>(exp_base);
              if(value > bound) {
                throw do_make_parser_error(reader, tlen, Parser_Error::code_integer_literal_overflow);
              }
              value *= static_cast<unsigned>(exp_base);
            }
          }
          // Check for a previous sign symbol.
          auto qstok = do_check_mergeability(seq, reader);
          std::uint64_t imask = 0;
          if(qstok && (qstok->as_punctuator() == Token::punctuator_sub)) {
            imask = UINT64_MAX;
          }
          // The special value `0x1p63` is only allowed if a contiguous minus symbol precedes it.
          if((value == static_cast<std::uint64_t>(INT64_MIN)) && (imask == 0)) {
            throw do_make_parser_error(reader, tlen, Parser_Error::code_integer_literal_overflow);
          }
          if(qstok) {
            // Overwrite the previous token.
            tlen += reader.offset() - qstok->offset();
            reader.rewind(qstok->offset());
            qstok = nullptr;
            seq.pop_back();
          }
          // Push an integer literal.
          Token::S_integer_literal xtoken = { static_cast<std::int64_t>((value ^ imask) - imask) };
          do_push_token(seq, reader, tlen, rocket::move(xtoken));
          return true;
        }
        // Parse the literal as a floating-point number.
        // Parse the integral part.
        double value = 0;
        bool zero = true;
        for(auto i = int_begin; i != int_end; ++i) {
          auto dptr = std::char_traits<char>::find(s_digits, max_digits, bptr[i]);
          if(!dptr) {
            continue;
          }
          auto dvalue = static_cast<int>((dptr - s_digits) / 2);
          value = value * radix + dvalue;
          zero |= dvalue;
        }
        // Parse the fractional part.
        double frac = 0;
        for(auto i = frac_end - 1; i + 1 != frac_begin; --i) {
          auto dptr = std::char_traits<char>::find(s_digits, max_digits, bptr[i]);
          if(!dptr) {
            continue;
          }
          auto dvalue = static_cast<int>((dptr - s_digits) / 2);
          frac = (frac + dvalue) / radix;
          zero |= dvalue;
        }
        value += frac;
        // Raise the significant part to the power of `exp`.
        if(exp_base == FLT_RADIX) {
          value = std::scalbn(value, exp);
#if FLT_RADIX != 2
        } else if(exp_base == 2) {
          value = std::ldexp(value, exp);
#endif
        } else {
          value = value * std::pow(exp_base, exp);
        }
        // Check for overflow or underflow.
        int fpc = std::fpclassify(value);
        if(fpc == FP_INFINITE) {
          throw do_make_parser_error(reader, tlen, Parser_Error::code_real_literal_overflow);
        }
        if((fpc == FP_ZERO) && !zero) {
          throw do_make_parser_error(reader, tlen, Parser_Error::code_real_literal_underflow);
        }
        // Check for a previous sign symbol.
        auto qstok = do_check_mergeability(seq, reader);
        double fmask = 0;
        if(qstok && (qstok->as_punctuator() == Token::punctuator_sub)) {
          fmask = -1;
        }
        if(qstok) {
          // Overwrite the previous token.
          tlen += reader.offset() - qstok->offset();
          reader.rewind(qstok->offset());
          qstok = nullptr;
          seq.pop_back();
        }
        // Push a floating-point literal.
        Token::S_real_literal xtoken = { std::copysign(value, fmask) };
        do_push_token(seq, reader, tlen, rocket::move(xtoken));
        return true;
      }

    }  // namespace

bool Token_Stream::load(std::streambuf& cbuf, const Cow_String& file, const Parser_Options& options)
  {
    // This has to be done before anything else because of possibility of exceptions.
    this->m_stor = nullptr;
    // Store tokens parsed here in normal order.
    // We will have to reverse this sequence before storing it into `*this` if it is accepted.
    Cow_Vector<Token> seq;
    try {
      // Save the position of an unterminated block comment.
      Tack bcomm;
      // Read source code line by line.
      Line_Reader reader(cbuf, file);
      while(reader.advance()) {
        // Discard the first line if it looks like a shebang.
        if((reader.line() == 1) && (reader.size_avail() >= 2) && (std::char_traits<char>::compare(reader.data_avail(), "#!", 2) == 0)) {
          continue;
        }
        // Ensure this line is a valid UTF-8 string.
        while(reader.size_avail() != 0) {
          char32_t cp;
          auto bptr = reader.data_avail();
          auto tptr = bptr;
          if(!utf8_decode(cp, tptr, reader.size_avail())) {
            throw do_make_parser_error(reader, static_cast<std::size_t>(tptr - bptr), Parser_Error::code_utf8_sequence_invalid);
          }
          reader.consume(static_cast<std::size_t>(tptr - bptr));
        }
        reader.rewind();
        // Break this line down into tokens.
        while(reader.size_avail() != 0) {
          // Are we inside a block comment?
          if(bcomm) {
            // Search for the terminator of this block comment.
            static constexpr char s_bcomm_term[2] = { '*', '/' };
            auto bptr = reader.data_avail();
            auto eptr = bptr + reader.size_avail();
            auto tptr = std::search(bptr, eptr, s_bcomm_term, s_bcomm_term + 2);
            if(tptr == eptr) {
              // The block comment will not end in this line. Stop.
              reader.consume(reader.size_avail());
              break;
            }
            tptr += 2;
            // Finish this comment and resume from the end of it.
            bcomm.clear();
            reader.consume(static_cast<std::size_t>(tptr - bptr));
            continue;
          }
          // Read a character.
          auto head = reader.peek();
          if(std::char_traits<char>::find(" \t\v\f\r\n", 6, head) != nullptr) {
            // Skip a space.
            reader.consume(1);
            continue;
          }
          if(head == '/') {
            auto next = reader.peek(1);
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
          bool token_got = do_accept_punctuator(seq, reader) ||
                           do_accept_string_literal(seq, reader, '\"', true) ||
                           do_accept_string_literal(seq, reader, '\'', options.escapable_single_quote_string) ||
                           do_accept_identifier_or_keyword(seq, reader, options.keyword_as_identifier) ||
                           do_accept_numeric_literal(seq, reader, options.integer_as_real);
          if(!token_got) {
            ASTERIA_DEBUG_LOG("Non-token character encountered in source code: ", reader.data_avail());
            throw do_make_parser_error(reader, 1, Parser_Error::code_token_character_unrecognized);
          }
        }
        reader.rewind();
      }
      if(bcomm) {
        // A block comment may straddle multiple lines. We just mark the first line here.
        throw Parser_Error(bcomm.line(), bcomm.offset(), bcomm.length(), Parser_Error::code_block_comment_unclosed);
      }
    } catch(Parser_Error& err) {  // Don't play with this at home.
      ASTERIA_DEBUG_LOG("Caught `Parser_Error`:\n",
                        "line = ", err.line(), ", offset = ", err.offset(), ", length = ", err.length(), "\n",
                        "code = ", err.code(), ": ", Parser_Error::get_code_description(err.code()));
      this->m_stor = rocket::move(err);
      return false;
    }
    std::reverse(seq.mut_begin(), seq.mut_end());
    this->m_stor = rocket::move(seq);
    return true;
  }

void Token_Stream::clear() noexcept
  {
    this->m_stor = nullptr;
  }

Parser_Error Token_Stream::get_parser_error() const noexcept
  {
    switch(this->state()) {
    case state_empty:
      {
        return Parser_Error(0, 0, 0, Parser_Error::code_no_data_loaded);
      }
    case state_error:
      {
        return this->m_stor.as<Parser_Error>();
      }
    case state_success:
      {
        return Parser_Error(0, 0, 0, Parser_Error::code_success);
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
    }
  }

bool Token_Stream::empty() const noexcept
  {
    switch(this->state()) {
    case state_empty:
      {
        return true;
      }
    case state_error:
      {
        return true;
      }
    case state_success:
      {
        return this->m_stor.as<Cow_Vector<Token>>().empty();
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
    }
  }

const Token* Token_Stream::peek_opt() const
  {
    switch(this->state()) {
    case state_empty:
      {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
    case state_error:
      {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
    case state_success:
      {
        auto& altr = this->m_stor.as<Cow_Vector<Token>>();
        if(altr.empty()) {
          return nullptr;
        }
        return &(altr.back());
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
    }
  }

void Token_Stream::shift()
  {
    switch(this->state()) {
    case state_empty:
      {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
    case state_error:
      {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
    case state_success:
      {
        auto& altr = this->m_stor.as<Cow_Vector<Token>>();
        if(altr.empty()) {
          ASTERIA_THROW_RUNTIME_ERROR("There are no more tokens from this stream.");
        }
        altr.pop_back();
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
    }
  }

}  // namespace Asteria
