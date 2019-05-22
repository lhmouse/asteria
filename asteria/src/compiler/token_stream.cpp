// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "parser_error.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    enum : std::uint8_t
      {
        cctype_space   = 0x01,  // [ \t\v\f\r\n]
        cctype_alpha   = 0x02,  // [A-Za-z]
        cctype_digit   = 0x04,  // [0-9]
        cctype_xdigit  = 0x08,  // [0-9A-Fa-f]
        cctype_namei   = 0x10,  // [A-Za-z_]
      };

    constexpr std::uint8_t s_cctypes[128] =
      {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
        0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x12,
        0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
        0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
        0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x10,
        0x00, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x12,
        0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
        0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
        0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00,
      };

    inline bool do_check_cctype(char ch, std::uint8_t mask) noexcept
      {
        int index = ch & 0x7F;
        if(index != ch) {
          return false;
        }
        return s_cctypes[index] & mask;
      }

    constexpr std::uint8_t s_digits[128] =
      {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      };

    inline std::uint8_t do_translate_digit(char ch) noexcept
      {
        int index = ch & 0x7F;
        if(index != ch) {
          return 0xFF;
        }
        return s_digits[index];
      }

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
              if(ich == std::istream::traits_type::eof()) {
                if(this->m_str.empty()) {
                  // Return `false` to indicate that there are no more data, when nothing has been read so far.
                  return false;
                }
                // If the last line doesn't end with an LF, accept it anyway.
                break;
              }
              if(ich == '\n') {
                // Accept a line.
                break;
              }
              this->m_str.push_back(static_cast<char>(ich));
            }
            // Increment the line number if a line has been read successfully.
            this->m_line++;
            ASTERIA_DEBUG_LOG("Read line ", std::setw(4), this->m_line, ": ", this->m_str);
            return true;
          }

        std::size_t offset() const noexcept
          {
            return this->m_offset;
          }
        std::size_t navail() const noexcept
          {
            return this->m_str.size() - this->m_offset;
          }
        const char* data(std::size_t add = 0) const
          {
            if(add > this->m_str.size() - this->m_offset) {
              ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to seek past the end of the current line.");
            }
            return this->m_str.data() + (this->m_offset + add);
          }
        char peek(std::size_t add = 0) const noexcept
          {
            if(add > this->m_str.size() - this->m_offset) {
              return 0;
            }
            return this->m_str[this->m_offset + add];
          }
        void consume(std::size_t add)
          {
            if(add > this->m_str.size() - this->m_offset) {
              ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to seek past the end of the current line.");
            }
            this->m_offset += add;
          }
        void rewind(std::size_t off = 0)
          {
            if(off > this->m_str.size()) {
              ASTERIA_THROW_RUNTIME_ERROR("The offset was past the end of the current line.");
            }
            this->m_offset = off;
          }
      };

    inline Parser_Error do_make_parser_error(const Line_Reader& reader, std::size_t tlen, Parser_Error::Code code)
      {
        return Parser_Error(reader.line(), reader.offset(), tlen, code);
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

    template<typename XtokenT> void do_push_token(Cow_Vector<Token>& seq, Line_Reader& reader, std::size_t tlen, XtokenT&& xtoken)
      {
        seq.emplace_back(reader.file(), reader.line(), reader.offset(), tlen, rocket::forward<XtokenT>(xtoken));
        reader.consume(tlen);
      }

    inline bool do_accumulate_digit(std::int64_t& value, std::int64_t limit, std::uint8_t base, std::uint8_t dvalue) noexcept
      {
        auto sbtm = limit >> 63;
        if(sbtm ? (value < (limit + dvalue) / base) : (value > (limit - dvalue) / base)) {
          return false;
        }
        value *= base;
        value += dvalue ^ sbtm;
        value -= sbtm;
        return true;
      }

    inline void do_raise(double& value, std::uint8_t base, std::int64_t exp) noexcept
      {
        if(exp > 0) {
          value *= power_u64(base, +static_cast<std::uint64_t>(exp));
        }
        if(exp < 0) {
          value /= power_u64(base, -static_cast<std::uint64_t>(exp));
        }
      }

    bool do_may_infix_operators_follow(Cow_Vector<Token>& seq)
      {
        if(seq.empty()) {
          // No previous token exists.
          return false;
        }
        const auto& p = seq.back();
        if(p.is_keyword()) {
          // Infix operators may follow if the keyword denotes a value or reference.
          return rocket::is_any_of(p.as_keyword(),{ Token::keyword_null, Token::keyword_true, Token::keyword_false,
                                                    Token::keyword_nan, Token::keyword_infinity, Token::keyword_this });
        }
        if(p.is_punctuator()) {
          // Infix operators may follow if the punctuator can terminate an expression.
          return rocket::is_any_of(p.as_punctuator(), { Token::punctuator_inc, Token::punctuator_dec,
                                                        Token::punctuator_parenth_cl, Token::punctuator_bracket_cl,
                                                        Token::punctuator_brace_cl });
        }
        // Infix operators can follow.
        return true;
      }

    bool do_accept_numeric_literal(Cow_Vector<Token>& seq, Line_Reader& reader, bool integer_as_real)
      {
        // numeric-literal ::=
        //   number-sign-opt ( binary-literal | decimal-literal | hexadecimal-literal ) exponent-suffix-opt
        // number-sign-opt ::=
        //   PCRE([+-]?)
        // binary-literal ::=
        //   PCRE(0[bB]([01]`?)+(\.([01]`?)+))
        // decimal-literal ::=
        //   PCRE(([0-9]`?)+(\.([0-9]`?)+))
        // hexadecimal-literal ::=
        //   PCRE(0[xX]([0-9A-Fa-f]`?)+(\.([0-9A-Fa-f]`?)+))
        // exponent-suffix-opt ::=
        //   decimal-exponent-suffix | binary-exponent-suffix | ""
        // decimal-exponent-suffix ::=
        //   PCRE([eE][-+]?([0-9]`?)+)
        // binary-exponent-suffix ::=
        //   PCRE([pP][-+]?([0-9]`?)+)
        bool rneg = false;  // is the number negative?
        std::size_t rbegin = 0;  // beginning of significant figures
        std::size_t rend = 0;  // end of significant figures
        std::uint8_t rbase = 10;  // the base of the integral and fractional parts.
        std::int64_t icnt = 0;  // number of integral digits (always non-negative)
        std::int64_t fcnt = 0;  // number of fractional digits (always non-negative)
        std::uint8_t pbase = 0;  // the base of the exponent.
        std::int64_t pexp = 0;  // `pbase`'d exponent
        // Get the sign of the number if any.
        std::size_t tlen = 0;
        switch(reader.peek(tlen)) {
        case '+':
          tlen++;
          rneg = false;
          break;
        case '-':
          tlen++;
          rneg = true;
          break;
        }
        if((tlen != 0) && do_may_infix_operators_follow(seq)) {
          return false;
        }
        if(!do_check_cctype(reader.peek(tlen), cctype_digit)) {
          return false;
        }
        // Check for the base prefix.
        if(reader.peek(tlen) == '0') {
          switch(reader.peek(tlen + 1)) {
          case 'b':
          case 'B':
            tlen += 2;
            rbase = 2;
            break;
          case 'x':
          case 'X':
            tlen += 2;
            rbase = 16;
            break;
          }
        }
        rbegin = tlen;
        rend = tlen;
        // Parse the integral part.
        for(;;) {
          auto dvalue = do_translate_digit(reader.peek(tlen));
          if(dvalue >= rbase) {
            break;
          }
          tlen++;
          // Accept a digit.
          rend = tlen;
          icnt++;
          // Is the next character a digit separator?
          if(reader.peek(tlen) == '`') {
            tlen++;
          }
        }
        // There shall be at least one digit.
        if(icnt == 0) {
          throw do_make_parser_error(reader, tlen, Parser_Error::code_numeric_literal_incomplete);
        }
        // Check for the fractional part.
        if(reader.peek(tlen) == '.') {
          tlen++;
          // Parse the fractional part.
          for(;;) {
            auto dvalue = do_translate_digit(reader.peek(tlen));
            if(dvalue >= rbase) {
              break;
            }
            tlen++;
            // Accept a digit.
            rend = tlen;
            fcnt++;
            // Is the next character a digit separator?
            if(reader.peek(tlen) == '`') {
              tlen++;
            }
          }
          // There shall be at least one digit.
          if(fcnt == 0) {
            throw do_make_parser_error(reader, tlen, Parser_Error::code_numeric_literal_incomplete);
          }
        }
        // Check for the exponent part.
        switch(reader.peek(tlen)) {
        case 'e':
        case 'E':
          tlen++;
          pbase = 10;
          break;
        case 'p':
        case 'P':
          tlen++;
          pbase = 2;
          break;
        }
        if(pbase != 0) {
          // Parse the exponent part.
          bool pneg = false;  // is the exponent negative?
          std::int64_t ecnt = 0;  // number of exponent digits (always non-negative)
          // Get the sign of the exponent if any.
          switch(reader.peek(tlen)) {
          case '+':
            tlen++;
            pneg = false;
            break;
          case '-':
            tlen++;
            pneg = true;
            break;
          }
          // Parse the exponent as an integer. The value must fit in 24 bits.
          for(;;) {
            auto dvalue = do_translate_digit(reader.peek(tlen));
            if(dvalue >= 10) {
              break;
            }
            tlen++;
            // Accept a digit.
            if(!do_accumulate_digit(pexp, pneg ? -0x800000 : +0x7FFFFF, 10, dvalue)) {
              throw do_make_parser_error(reader, tlen, Parser_Error::code_numeric_literal_exponent_overflow);
            }
            ecnt++;
            // Is the next character a digit separator?
            if(reader.peek(tlen) == '`') {
              tlen++;
            }
          }
          // There shall be at least one digit.
          if(ecnt == 0) {
            throw do_make_parser_error(reader, tlen, Parser_Error::code_numeric_literal_incomplete);
          }
        }
        if(reader.peek(tlen) == '`') {
          throw do_make_parser_error(reader, tlen, Parser_Error::code_digit_separator_following_nondigit);
        }
        // Disallow suffixes. Suffixes such as `ll`, `u` and `f` are used in C and C++ to specify the types of numeric literals.
        // Since we make no use of them, we just reserve them for further use for good.
        if(do_check_cctype(reader.peek(tlen), cctype_namei | cctype_digit)) {
          throw do_make_parser_error(reader, tlen, Parser_Error::code_numeric_literal_suffix_disallowed);
        }
        // Is this an `integer` or a `real`?
        if(!integer_as_real && (fcnt == 0)) {
          // The literal is an `integer` if there is no decimal point.
          std::int64_t value = 0;
          // Accumulate digits from left to right.
          for(auto ri = rbegin; ri != rend; ++ri) {
            auto dvalue = do_translate_digit(reader.peek(ri));
            if(dvalue >= rbase) {
              continue;
            }
            // Accept a digit.
            if(!do_accumulate_digit(value, rneg ? INT64_MIN : INT64_MAX, rbase, dvalue)) {
              throw do_make_parser_error(reader, tlen, Parser_Error::code_integer_literal_overflow);
            }
          }
          // Negative exponents are not allowed, not even when the significant part is zero.
          if(pexp < 0) {
            throw do_make_parser_error(reader, tlen, Parser_Error::code_integer_literal_exponent_negative);
          }
          // Raise the result.
          if(value != 0) {
            for(auto i = pexp; i != 0; --i) {
              // Append a digit zero.
              if(!do_accumulate_digit(value, rneg ? INT64_MIN : INT64_MAX, pbase, 0)) {
                throw do_make_parser_error(reader, tlen, Parser_Error::code_integer_literal_overflow);
              }
            }
          }
          // Push an integer literal.
          Token::S_integer_literal xtoken = { value };
          do_push_token(seq, reader, tlen, rocket::move(xtoken));
          return true;
        }
        // The literal is a `real` if there is a decimal point.
        if(rbase == pbase) {
          // Contract floating operations to minimize rounding errors.
          pexp -= fcnt;
          icnt += fcnt;
          fcnt = 0;
        }
        // Digits are accumulated using a 64-bit integer with no fractional part.
        // Excess significant figures are discard if the integer would overflow.
        std::int64_t tvalue = 0;
        std::int64_t tcnt = icnt;
        // Accumulate digits from left to right.
        for(auto ri = rbegin; ri != rend; ++ri) {
          auto dvalue = do_translate_digit(reader.peek(ri));
          if(dvalue >= rbase) {
            continue;
          }
          // Accept a digit.
          if(!do_accumulate_digit(tvalue, rneg ? INT64_MIN : INT64_MAX, rbase, dvalue)) {
            break;
          }
          // Nudge the decimal point to the right.
          tcnt--;
        }
        // Raise the result.
        double value;
        if(tvalue == 0) {
          value = std::copysign(0.0, -rneg);
        } else {
          value = static_cast<double>(tvalue);
          do_raise(value, rbase, tcnt);
          do_raise(value, pbase, pexp);
        }
        // Check for overflow or underflow.
        int fpcls = std::fpclassify(value);
        if(fpcls == FP_INFINITE) {
          throw do_make_parser_error(reader, tlen, Parser_Error::code_real_literal_overflow);
        }
        if((fpcls == FP_ZERO) && (tvalue != 0)) {
          throw do_make_parser_error(reader, tlen, Parser_Error::code_real_literal_underflow);
        }
        // Push a real literal.
        Token::S_real_literal xtoken = { value };
        do_push_token(seq, reader, tlen, rocket::move(xtoken));
        return true;
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
        auto range = std::equal_range(std::begin(s_punctuators), std::end(s_punctuators), reader.peek(), Prefix_Comparator());
        for(;;) {
          if(range.first == range.second) {
            // No matching punctuator has been found so far.
            return false;
          }
          const auto& cur = range.second[-1];
          // Has a match been found?
          auto tlen = std::strlen(cur.first);
          if((tlen <= reader.navail()) && (std::memcmp(reader.data(), cur.first, tlen) == 0)) {
            // A punctuator has been found.
            Token::S_punctuator xtoken = { cur.second };
            do_push_token(seq, reader, tlen, rocket::move(xtoken));
            return true;
          }
          range.second--;
        }
      }

    bool do_accept_string_literal(Cow_Vector<Token>& seq, Line_Reader& reader, char head, bool escapable)
      {
        // string-literal ::=
        //   PCRE("([^\\]|(\\([abfnrtveZ0'"?\\]|(x[0-9A-Fa-f]{2})|(u[0-9A-Fa-f]{4})|(U[0-9A-Fa-f]{6}))))*?")
        // noescape-string-literal ::=
        //   PCRE('[^']*?')
        if(reader.peek() != head) {
          return false;
        }
        // Get a string literal.
        std::size_t tlen = 1;
        Cow_String value;
        for(;;) {
          // Read a character.
          auto next = reader.peek(tlen);
          if(next == 0) {
            throw do_make_parser_error(reader, tlen, Parser_Error::code_string_literal_unclosed);
          }
          tlen++;
          // Check it.
          if(next == head) {
            // The end of this string is encountered. Finish.
            break;
          }
          if(!escapable || (next != '\\')) {
            // This character does not start an escape sequence. Copy it as is.
            value.push_back(next);
            continue;
          }
          // Translate this escape sequence.
          // Read the next charactter.
          next = reader.peek(tlen);
          if(next == 0) {
            throw do_make_parser_error(reader, tlen, Parser_Error::code_escape_sequence_incomplete);
          }
          tlen++;
          // Translate it.
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
          case 'u':
          case 'x':
            {
              char32_t cp = 0;
              // How many hex digits are there?
              int xcnt = 2 + (next != 'x') * 2 + (next == 'U') * 2;
              // Read hex digits.
              for(int i = 0; i < xcnt; ++i) {
                // Read a hex digit.
                auto digit = reader.peek(tlen);
                if(digit == 0) {
                  throw do_make_parser_error(reader, tlen, Parser_Error::code_escape_sequence_incomplete);
                }
                auto dvalue = do_translate_digit(digit);
                if(dvalue >= 16) {
                  throw do_make_parser_error(reader, tlen, Parser_Error::code_escape_sequence_invalid_hex);
                }
                tlen++;
                // Accumulate this digit.
                cp *= 16;
                cp += dvalue;
              }
              if(next == 'x') {
                // Write the character verbatim.
                value.push_back(static_cast<char>(cp));
                break;
              }
              // Write a Unicode code point.
              if(!utf8_encode(value, cp)) {
                throw do_make_parser_error(reader, tlen, Parser_Error::code_escape_utf_code_point_invalid);
              }
              break;
            }
          default:
            throw do_make_parser_error(reader, tlen, Parser_Error::code_escape_sequence_unknown);
          }
        }
        Token::S_string_literal xtoken = { rocket::move(value) };
        do_push_token(seq, reader, tlen, rocket::move(xtoken));
        return true;
      }

    bool do_accept_identifier_or_keyword(Cow_Vector<Token>& seq, Line_Reader& reader, bool keyword_as_identifier)
      {
        // identifier ::=
        //   PCRE([A-Za-z_][A-Za-z_0-9]*)
        if(!do_check_cctype(reader.peek(), cctype_namei)) {
          return false;
        }
        // Get the length of this identifier.
        std::size_t tlen = 1;
        for(;;) {
          auto next = reader.peek(tlen);
          if(next == 0) {
            break;
          }
          if(!do_check_cctype(next, cctype_namei | cctype_digit)) {
            break;
          }
          ++tlen;
        }
        if(keyword_as_identifier) {
          // Do not check for identifiers.
          Token::S_identifier xtoken = { Cow_String(reader.data(), tlen) };
          do_push_token(seq, reader, tlen, rocket::move(xtoken));
          return true;
        }
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
            { "__fma",     Token::keyword_fma       },
            { "__iceil",   Token::keyword_iceil     },
            { "__ifloor",  Token::keyword_ifloor    },
            { "__iround",  Token::keyword_iround    },
            { "__isinf",   Token::keyword_isinf     },
            { "__isnan",   Token::keyword_isnan     },
            { "__itrunc",  Token::keyword_itrunc    },
            { "__round",   Token::keyword_round     },
            { "__signb",   Token::keyword_signb     },
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
        auto range = std::equal_range(std::begin(s_keywords), std::end(s_keywords), reader.peek(), Prefix_Comparator());
        for(;;) {
          if(range.first == range.second) {
            // No matching keyword has been found so far.
            Token::S_identifier xtoken = { Cow_String(reader.data(), tlen) };
            do_push_token(seq, reader, tlen, rocket::move(xtoken));
            return true;
          }
          const auto& cur = range.first[0];
          if((std::strlen(cur.first) == tlen) && (std::memcmp(reader.data(), cur.first, tlen) == 0)) {
            // A keyword has been found.
            Token::S_keyword xtoken = { cur.second };
            do_push_token(seq, reader, tlen, rocket::move(xtoken));
            return true;
          }
          range.first++;
        }
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
        if((reader.line() == 1) && (reader.navail() >= 2) && (std::memcmp(reader.data(), "#!", 2) == 0)) {
          continue;
        }
        // Ensure this line is a valid UTF-8 string.
        while(reader.navail() != 0) {
          // Decode a code point.
          char32_t cp;
          auto tptr = reader.data();
          if(!utf8_decode(cp, tptr, reader.navail())) {
            throw do_make_parser_error(reader, reader.navail(), Parser_Error::code_utf8_sequence_invalid);
          }
          auto u8len = static_cast<std::size_t>(tptr - reader.data());
          // Disallow plain null characters in source data.
          if(cp == 0) {
            throw do_make_parser_error(reader, u8len, Parser_Error::code_null_character_disallowed);
          }
          // Accept this code point.
          reader.consume(u8len);
        }
        reader.rewind();
        // Break this line down into tokens.
        while(reader.navail() != 0) {
          // Are we inside a block comment?
          if(bcomm) {
            // Search for the terminator of this block comment.
            auto tptr = std::strstr(reader.data(), "*/");
            if(!tptr) {
              // The block comment will not end in this line. Stop.
              break;
            }
            auto tlen = static_cast<std::size_t>(tptr + 2 - reader.data());
            // Finish this comment and resume from the end of it.
            bcomm.clear();
            reader.consume(tlen);
            continue;
          }
          // Read a character.
          if(do_check_cctype(reader.peek(), cctype_space)) {
            // Skip a space.
            reader.consume(1);
            continue;
          }
          if(reader.peek() == '/') {
            if(reader.peek(1) == '/') {
              // Start a line comment. Discard all remaining characters in this line.
              break;
            }
            if(reader.peek(1) == '*') {
              // Start a block comment.
              bcomm.set(reader, 2);
              reader.consume(2);
              continue;
            }
          }
          bool token_got = do_accept_numeric_literal(seq, reader, options.integer_as_real) ||
                           do_accept_punctuator(seq, reader) ||
                           do_accept_string_literal(seq, reader, '\"', true) ||
                           do_accept_string_literal(seq, reader, '\'', options.escapable_single_quote_string) ||
                           do_accept_identifier_or_keyword(seq, reader, options.keyword_as_identifier);
          if(!token_got) {
            ASTERIA_DEBUG_LOG("Non-token character encountered in source code: ", reader.data());
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
