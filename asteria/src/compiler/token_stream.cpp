// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "parser_error.hpp"
#include "../utilities.hpp"

namespace Asteria {

    namespace {

    enum : uint8_t
      {
        cctype_space   = 0x01,  // [ \t\v\f\r\n]
        cctype_alpha   = 0x02,  // [A-Za-z]
        cctype_digit   = 0x04,  // [0-9]
        cctype_xdigit  = 0x08,  // [0-9A-Fa-f]
        cctype_namei   = 0x10,  // [A-Za-z_]
      };

    constexpr uint8_t s_cctypes[128] =
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

    inline bool do_check_cctype(char c, uint8_t mask) noexcept
      {
        int index = c & 0x7F;
        if(index != c) {
          return false;
        }
        return s_cctypes[index] & mask;
      }

    constexpr uint8_t s_digits[128] =
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

    inline uint8_t do_translate_digit(char c) noexcept
      {
        int index = c & 0x7F;
        if(index != c) {
          return 0xFF;
        }
        return s_digits[index];
      }

    class Line_Reader
      {
      private:
        std::reference_wrapper<std::streambuf> m_sbuf;
        cow_string m_file;

        cow_string m_str;
        int64_t m_line;
        size_t m_offset;

      public:
        Line_Reader(std::streambuf& xsbuf, const cow_string& xfile)
          : m_sbuf(xsbuf), m_file(xfile),
            m_str(), m_line(0), m_offset(0)
          {
          }

        Line_Reader(const Line_Reader&)
          = delete;
        Line_Reader& operator=(const Line_Reader&)
          = delete;

      public:
        std::streambuf& sbuf() const noexcept
          {
            return this->m_sbuf;
          }
        const cow_string& file() const noexcept
          {
            return this->m_file;
          }

        int64_t line() const noexcept
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
              auto ch = this->m_sbuf.get().sbumpc();
              if(ch == std::istream::traits_type::eof()) {
                if(this->m_str.empty()) {
                  // Return `false` to indicate that there are no more data, when nothing has been read so far.
                  return false;
                }
                // If the last line doesn't end with an LF, accept it anyway.
                break;
              }
              if(ch == '\n') {
                // Accept a line.
                break;
              }
              this->m_str.push_back(static_cast<char>(ch));
            }
            // Increment the line number if a line has been read successfully.
            this->m_line++;
            ASTERIA_DEBUG_LOG("Read line ", std::setw(4), this->m_line, ": ", this->m_str);
            return true;
          }

        size_t offset() const noexcept
          {
            return this->m_offset;
          }
        size_t navail() const noexcept
          {
            return this->m_str.size() - this->m_offset;
          }
        const char* data(size_t add = 0) const
          {
            if(add > this->m_str.size() - this->m_offset) {
              ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to seek past the end of the current line.");
            }
            return this->m_str.data() + (this->m_offset + add);
          }
        char peek(size_t add = 0) const noexcept
          {
            if(add > this->m_str.size() - this->m_offset) {
              return 0;
            }
            return this->m_str[this->m_offset + add];
          }
        void consume(size_t add)
          {
            if(add > this->m_str.size() - this->m_offset) {
              ASTERIA_THROW_RUNTIME_ERROR("An attempt was made to seek past the end of the current line.");
            }
            this->m_offset += add;
          }
        void rewind(size_t off = 0)
          {
            if(off > this->m_str.size()) {
              ASTERIA_THROW_RUNTIME_ERROR("The offset was past the end of the current line.");
            }
            this->m_offset = off;
          }
      };

    inline Parser_Error do_make_parser_error(const Line_Reader& reader, size_t tlen, Parser_Status status)
      {
        return Parser_Error(reader.line(), reader.offset(), tlen, status);
      }

    class Tack
      {
      private:
        int64_t m_line;
        size_t m_offset;
        size_t m_length;

      public:
        constexpr Tack() noexcept
          : m_line(0), m_offset(0), m_length(0)
          {
          }

      public:
        constexpr int64_t line() const noexcept
          {
            return this->m_line;
          }
        constexpr size_t offset() const noexcept
          {
            return this->m_offset;
          }
        constexpr size_t length() const noexcept
          {
            return this->m_length;
          }
        explicit constexpr operator bool () const noexcept
          {
            return this->m_line != 0;
          }
        Tack& set(const Line_Reader& reader, size_t xlength) noexcept
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

    template<typename XtokenT> void do_push_token(cow_vector<Token>& seq, Line_Reader& reader, size_t tlen, XtokenT&& xtoken)
      {
        seq.emplace_back(reader.file(), reader.line(), reader.offset(), tlen, rocket::forward<XtokenT>(xtoken));
        reader.consume(tlen);
      }

    inline bool do_accumulate_digit(int64_t& val, int64_t limit, uint8_t base, uint8_t dval) noexcept
      {
        if(limit >= 0) {
          // Accumulate the digit towards positive infinity.
          if(val > (limit - dval) / base) {
            return false;
          }
          val *= base;
          val += dval;
        }
        else {
          // Accumulate the digit towards negative infinity.
          if(val < (limit + dval) / base) {
            return false;
          }
          val *= base;
          val -= dval;
        }
        return true;
      }

    inline void do_raise(double& val, uint8_t base, int64_t exp) noexcept
      {
        if(exp > 0) {
          val *= std::pow(base, +exp);
        }
        if(exp < 0) {
          val /= std::pow(base, -exp);
        }
      }

    bool do_may_infix_operators_follow(cow_vector<Token>& seq)
      {
        if(seq.empty()) {
          // No previous token exists.
          return false;
        }
        const auto& p = seq.back();
        if(p.is_keyword()) {
          // Infix operators may follow if the keyword denotes a value or reference.
          return rocket::is_any_of(p.as_keyword(),{ keyword_null, keyword_true, keyword_false,
                                                    keyword_nan, keyword_infinity, keyword_this });
        }
        if(p.is_punctuator()) {
          // Infix operators may follow if the punctuator can terminate an expression.
          return rocket::is_any_of(p.as_punctuator(), { punctuator_inc, punctuator_dec,
                                                        punctuator_parenth_cl, punctuator_bracket_cl,
                                                        punctuator_brace_cl });
        }
        // Infix operators can follow.
        return true;
      }

    bool do_accept_numeric_literal(cow_vector<Token>& seq, Line_Reader& reader, bool integers_as_reals)
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
        size_t rbegin = 0;  // beginning of significant figures
        size_t rend = 0;  // end of significant figures
        uint8_t rbase = 10;  // the base of the integral and fractional parts.
        int64_t icnt = 0;  // number of integral digits (always non-negative)
        int64_t fcnt = 0;  // number of fractional digits (always non-negative)
        uint8_t pbase = 0;  // the base of the exponent.
        bool pneg = false;  // is the exponent negative?
        int64_t pexp = 0;  // `pbase`'d exponent
        int64_t pcnt = 0;  // number of exponent digits (always non-negative)
        // Get the sign of the number if any.
        size_t tlen = 0;
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
          auto dval = do_translate_digit(reader.peek(tlen));
          if(dval >= rbase) {
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
          throw do_make_parser_error(reader, tlen, parser_status_numeric_literal_incomplete);
        }
        // Check for the fractional part.
        if(reader.peek(tlen) == '.') {
          tlen++;
          // Parse the fractional part.
          for(;;) {
            auto dval = do_translate_digit(reader.peek(tlen));
            if(dval >= rbase) {
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
            throw do_make_parser_error(reader, tlen, parser_status_numeric_literal_incomplete);
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
            auto dval = do_translate_digit(reader.peek(tlen));
            if(dval >= 10) {
              break;
            }
            tlen++;
            // Accept a digit.
            if(!do_accumulate_digit(pexp, pneg ? -0x800000 : +0x7FFFFF, 10, dval)) {
              throw do_make_parser_error(reader, tlen, parser_status_numeric_literal_exponent_overflow);
            }
            pcnt++;
            // Is the next character a digit separator?
            if(reader.peek(tlen) == '`') {
              tlen++;
            }
          }
          // There shall be at least one digit.
          if(pcnt == 0) {
            throw do_make_parser_error(reader, tlen, parser_status_numeric_literal_incomplete);
          }
        }
        if(reader.peek(tlen) == '`') {
          throw do_make_parser_error(reader, tlen, parser_status_digit_separator_following_nondigit);
        }
        // Disallow suffixes. Suffixes such as `ll`, `u` and `f` are used in C and C++ to specify the types of numeric literals.
        // Since we make no use of them, we just reserve them for further use for good.
        if(do_check_cctype(reader.peek(tlen), cctype_namei | cctype_digit)) {
          throw do_make_parser_error(reader, tlen, parser_status_numeric_literal_suffix_disallowed);
        }
        // Is this an `integer` or a `real`?
        if(!integers_as_reals && (fcnt == 0)) {
          // The literal is an `integer` if there is no decimal point.
          int64_t val = 0;
          // Accumulate digits from left to right.
          for(auto ri = rbegin; ri != rend; ++ri) {
            auto dval = do_translate_digit(reader.peek(ri));
            if(dval >= rbase) {
              continue;
            }
            // Accept a digit.
            if(!do_accumulate_digit(val, rneg ? INT64_MIN : INT64_MAX, rbase, dval)) {
              throw do_make_parser_error(reader, tlen, parser_status_integer_literal_overflow);
            }
          }
          // Negative exponents are not allowed, not even when the significant part is zero.
          if(pexp < 0) {
            throw do_make_parser_error(reader, tlen, parser_status_integer_literal_exponent_negative);
          }
          // Raise the result.
          if(val != 0) {
            for(auto i = pexp; i != 0; --i) {
              // Append a digit zero.
              if(!do_accumulate_digit(val, rneg ? INT64_MIN : INT64_MAX, pbase, 0)) {
                throw do_make_parser_error(reader, tlen, parser_status_integer_literal_overflow);
              }
            }
          }
          // Push an integer literal.
          Token::S_integer_literal xtoken = { val };
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
        int64_t tval = 0;
        int64_t tcnt = icnt;
        // Accumulate digits from left to right.
        for(auto ri = rbegin; ri != rend; ++ri) {
          auto dval = do_translate_digit(reader.peek(ri));
          if(dval >= rbase) {
            continue;
          }
          // Accept a digit.
          if(!do_accumulate_digit(tval, rneg ? INT64_MIN : INT64_MAX, rbase, dval)) {
            break;
          }
          // Nudge the decimal point to the right.
          tcnt--;
        }
        // Raise the result.
        double val;
        if(tval == 0) {
          val = std::copysign(0.0, -rneg);
        }
        else {
          val = static_cast<double>(tval);
          do_raise(val, rbase, tcnt);
          do_raise(val, pbase, pexp);
        }
        // Check for overflow or underflow.
        int fpcls = std::fpclassify(val);
        if(fpcls == FP_INFINITE) {
          throw do_make_parser_error(reader, tlen, parser_status_real_literal_overflow);
        }
        if((fpcls == FP_ZERO) && (tval != 0)) {
          throw do_make_parser_error(reader, tlen, parser_status_real_literal_underflow);
        }
        // Push a real literal.
        Token::S_real_literal xtoken = { val };
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

    struct Punctuator_Element
      {
        char first[6];
        Punctuator second;
      }
    constexpr s_punctuators[] =
      {
        { "!",     punctuator_notl        },
        { "!=",    punctuator_cmp_ne      },
        { "%",     punctuator_mod         },
        { "%=",    punctuator_mod_eq      },
        { "&",     punctuator_andb        },
        { "&&",    punctuator_andl        },
        { "&&=",   punctuator_andl_eq     },
        { "&=",    punctuator_andb_eq     },
        { "(",     punctuator_parenth_op  },
        { ")",     punctuator_parenth_cl  },
        { "*",     punctuator_mul         },
        { "*=",    punctuator_mul_eq      },
        { "+",     punctuator_add         },
        { "++",    punctuator_inc         },
        { "+=",    punctuator_add_eq      },
        { ",",     punctuator_comma       },
        { "-",     punctuator_sub         },
        { "--",    punctuator_dec         },
        { "-=",    punctuator_sub_eq      },
        { ".",     punctuator_dot         },
        { "...",   punctuator_ellipsis    },
        { "/",     punctuator_div         },
        { "/=",    punctuator_div_eq      },
        { ":",     punctuator_colon       },
        { ";",     punctuator_semicol     },
        { "<",     punctuator_cmp_lt      },
        { "<<",    punctuator_sla         },
        { "<<<",   punctuator_sll         },
        { "<<<=",  punctuator_sll_eq      },
        { "<<=",   punctuator_sla_eq      },
        { "<=",    punctuator_cmp_lte     },
        { "<=>",   punctuator_spaceship   },
        { "=",     punctuator_assign      },
        { "==",    punctuator_cmp_eq      },
        { ">",     punctuator_cmp_gt      },
        { ">=",    punctuator_cmp_gte     },
        { ">>",    punctuator_sra         },
        { ">>=",   punctuator_sra_eq      },
        { ">>>",   punctuator_srl         },
        { ">>>=",  punctuator_srl_eq      },
        { "?",     punctuator_quest       },
        { "?=",    punctuator_quest_eq    },
        { "?\?",   punctuator_coales      },
        { "?\?=",  punctuator_coales_eq   },
        { "[",     punctuator_bracket_op  },
        { "]",     punctuator_bracket_cl  },
        { "^",     punctuator_xorb        },
        { "^=",    punctuator_xorb_eq     },
        { "{",     punctuator_brace_op    },
        { "|",     punctuator_orb         },
        { "|=",    punctuator_orb_eq      },
        { "||",    punctuator_orl         },
        { "||=",   punctuator_orl_eq      },
        { "}",     punctuator_brace_cl    },
        { "~",     punctuator_notb        },
      };

    bool do_accept_punctuator(cow_vector<Token>& seq, Line_Reader& reader)
      {
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

    bool do_accept_string_literal(cow_vector<Token>& seq, Line_Reader& reader, char head, bool escapable)
      {
        // string-literal ::=
        //   escape-string-literal | noescape-string-literal
        // escape-string-literal ::=
        //   PCRE("([^\\]|(\\([abfnrtveZ0'"?\\/]|(x[0-9A-Fa-f]{2})|(u[0-9A-Fa-f]{4})|(U[0-9A-Fa-f]{6}))))*?")
        // noescape-string-literal ::=
        //   PCRE('[^']*?')
        if(reader.peek() != head) {
          return false;
        }
        // Get a string literal.
        size_t tlen = 1;
        cow_string val;
        for(;;) {
          // Read a character.
          auto next = reader.peek(tlen);
          if(next == 0) {
            throw do_make_parser_error(reader, tlen, parser_status_string_literal_unclosed);
          }
          tlen++;
          // Check it.
          if(next == head) {
            // The end of this string is encountered. Finish.
            break;
          }
          if(!escapable || (next != '\\')) {
            // This character does not start an escape sequence. Copy it as is.
            val.push_back(next);
            continue;
          }
          // Translate this escape sequence.
          // Read the next charactter.
          next = reader.peek(tlen);
          if(next == 0) {
            throw do_make_parser_error(reader, tlen, parser_status_escape_sequence_incomplete);
          }
          tlen++;
          // Translate it.
          switch(next) {
          case '\'':
          case '\"':
          case '\\':
          case '?':
          case '/':
            {
              val.push_back(next);
              break;
            }
          case 'a':
            {
              val.push_back('\a');
              break;
            }
          case 'b':
            {
              val.push_back('\b');
              break;
            }
          case 'f':
            {
              val.push_back('\f');
              break;
            }
          case 'n':
            {
              val.push_back('\n');
              break;
            }
          case 'r':
            {
              val.push_back('\r');
              break;
            }
          case 't':
            {
              val.push_back('\t');
              break;
            }
          case 'v':
            {
              val.push_back('\v');
              break;
            }
          case '0':
            {
              val.push_back('\0');
              break;
            }
          case 'Z':
            {
              val.push_back('\x1A');
              break;
            }
          case 'e':
            {
              val.push_back('\x1B');
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
                  throw do_make_parser_error(reader, tlen, parser_status_escape_sequence_incomplete);
                }
                auto dval = do_translate_digit(digit);
                if(dval >= 16) {
                  throw do_make_parser_error(reader, tlen, parser_status_escape_sequence_invalid_hex);
                }
                tlen++;
                // Accumulate this digit.
                cp *= 16;
                cp += dval;
              }
              if(next == 'x') {
                // Write the character verbatim.
                val.push_back(static_cast<char>(cp));
                break;
              }
              // Write a Unicode code point.
              if(!utf8_encode(val, cp)) {
                throw do_make_parser_error(reader, tlen, parser_status_escape_utf_code_point_invalid);
              }
              break;
            }
          default:
            throw do_make_parser_error(reader, tlen, parser_status_escape_sequence_unknown);
          }
        }
        Token::S_string_literal xtoken = { rocket::move(val) };
        do_push_token(seq, reader, tlen, rocket::move(xtoken));
        return true;
      }

    struct Keyword_Element
      {
        char first[10];
        Keyword second;
      }
    constexpr s_keywords[] =
      {
        { "__abs",     keyword_abs       },
        { "__ceil",    keyword_ceil      },
        { "__floor",   keyword_floor     },
        { "__fma",     keyword_fma       },
        { "__iceil",   keyword_iceil     },
        { "__ifloor",  keyword_ifloor    },
        { "__iround",  keyword_iround    },
        { "__isinf",   keyword_isinf     },
        { "__isnan",   keyword_isnan     },
        { "__itrunc",  keyword_itrunc    },
        { "__round",   keyword_round     },
        { "__signb",   keyword_signb     },
        { "__sqrt",    keyword_sqrt      },
        { "__trunc",   keyword_trunc     },
        { "and",       keyword_and       },
        { "assert",    keyword_assert    },
        { "break",     keyword_break     },
        { "case",      keyword_case      },
        { "catch",     keyword_catch     },
        { "const",     keyword_const     },
        { "continue",  keyword_continue  },
        { "default",   keyword_default   },
        { "defer",     keyword_defer     },
        { "do",        keyword_do        },
        { "each",      keyword_each      },
        { "else",      keyword_else      },
        { "false",     keyword_false     },
        { "for",       keyword_for       },
        { "func",      keyword_func      },
        { "if",        keyword_if        },
        { "infinity",  keyword_infinity  },
        { "lengthof",  keyword_lengthof  },
        { "nan",       keyword_nan       },
        { "not",       keyword_not       },
        { "null",      keyword_null      },
        { "or",        keyword_or        },
        { "return",    keyword_return    },
        { "switch",    keyword_switch    },
        { "this",      keyword_this      },
        { "throw",     keyword_throw     },
        { "true",      keyword_true      },
        { "try",       keyword_try       },
        { "typeof",    keyword_typeof    },
        { "unset",     keyword_unset     },
        { "var",       keyword_var       },
        { "while",     keyword_while     },
      };

    bool do_accept_identifier_or_keyword(cow_vector<Token>& seq, Line_Reader& reader, bool keywords_as_identifiers)
      {
        // identifier ::=
        //   PCRE([A-Za-z_][A-Za-z_0-9]*)
        if(!do_check_cctype(reader.peek(), cctype_namei)) {
          return false;
        }
        // Get the length of this identifier.
        size_t tlen = 1;
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
        if(keywords_as_identifiers) {
          // Do not check for identifiers.
          Token::S_identifier xtoken = { cow_string(reader.data(), tlen) };
          do_push_token(seq, reader, tlen, rocket::move(xtoken));
          return true;
        }
#ifdef ROCKET_DEBUG
        ROCKET_ASSERT(std::is_sorted(std::begin(s_keywords), std::end(s_keywords), Prefix_Comparator()));
#endif
        auto range = std::equal_range(std::begin(s_keywords), std::end(s_keywords), reader.peek(), Prefix_Comparator());
        for(;;) {
          if(range.first == range.second) {
            // No matching keyword has been found so far.
            Token::S_identifier xtoken = { cow_string(reader.data(), tlen) };
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

bool Token_Stream::load(std::streambuf& sbuf, const cow_string& file, const Compiler_Options& options)
  {
    // This has to be done before anything else because of possibility of exceptions.
    this->m_stor = nullptr;
    // Store tokens parsed here in normal order.
    // We will have to reverse this sequence before storing it into `*this` if it is accepted.
    cow_vector<Token> seq;
    try {
      // Save the position of an unterminated block comment.
      Tack bcomm;
      // Read source code line by line.
      Line_Reader reader(sbuf, file);
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
            throw do_make_parser_error(reader, reader.navail(), parser_status_utf8_sequence_invalid);
          }
          auto u8len = static_cast<size_t>(tptr - reader.data());
          // Disallow plain null characters in source data.
          if(cp == 0) {
            throw do_make_parser_error(reader, u8len, parser_status_null_character_disallowed);
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
            auto tlen = static_cast<size_t>(tptr + 2 - reader.data());
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
          bool token_got = do_accept_numeric_literal(seq, reader, options.integers_as_reals) ||
                           do_accept_punctuator(seq, reader) ||
                           do_accept_string_literal(seq, reader, '\"', true) ||
                           do_accept_string_literal(seq, reader, '\'', options.escapable_single_quote_strings) ||
                           do_accept_identifier_or_keyword(seq, reader, options.keywords_as_identifiers);
          if(!token_got) {
            ASTERIA_DEBUG_LOG("Non-token character encountered in source code: ", reader.data());
            throw do_make_parser_error(reader, 1, parser_status_token_character_unrecognized);
          }
        }
        reader.rewind();
      }
      if(bcomm) {
        // A block comment may straddle multiple lines. We just mark the first line here.
        throw Parser_Error(bcomm.line(), bcomm.offset(), bcomm.length(), parser_status_block_comment_unclosed);
      }
    }
    catch(const Parser_Error& error) {  // `Parser_Error` is not derived from `std::exception`. Don't play with this at home.
      ASTERIA_DEBUG_LOG("Caught `Parser_Error`: ", error);
      this->m_stor = error;
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
        return Parser_Error(-1, SIZE_MAX, 0, parser_status_no_data_loaded);
      }
    case state_error:
      {
        return this->m_stor.as<Parser_Error>();
      }
    case state_success:
      {
        return Parser_Error(-1, SIZE_MAX, 0, parser_status_success);
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered. This is likely a bug. Please report.");
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
        return this->m_stor.as<cow_vector<Token>>().empty();
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

const Token* Token_Stream::peek_opt(size_t ahead) const
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
        auto& altr = this->m_stor.as<cow_vector<Token>>();
        if(ahead >= altr.size()) {
          return nullptr;
        }
        return altr.data() + (altr.size() - 1 - ahead);
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

void Token_Stream::shift(size_t count)
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
        auto& altr = this->m_stor.as<cow_vector<Token>>();
        if(count > altr.size()) {
          ASTERIA_THROW_RUNTIME_ERROR("There are no more tokens from this stream.");
        }
        altr.pop_back(count);
        return;
      }
    default:
      ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered. This is likely a bug. Please report.");
    }
  }

}  // namespace Asteria
