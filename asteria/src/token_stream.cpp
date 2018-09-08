// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "parser_result.hpp"
#include "utilities.hpp"
#include <algorithm>

namespace Asteria {

Token_stream::Token_stream() noexcept
  : m_rseq()
  {
  }
Token_stream::~Token_stream()
  {
  }

Token_stream::Token_stream(Token_stream &&) noexcept
  = default;
Token_stream & Token_stream::operator=(Token_stream &&) noexcept
  = default;

namespace {

  String & do_blank_comment(String &str_inout, Size tpos, Size tn)
    {
      auto pos = tpos;
      for(;;) {
        pos = str_inout.find_first_not_of(" \t\v\f\r\n", pos);
        if(pos - tpos >= tn) {
          break;
        }
        str_inout.mut(pos) = ' ';
        ++pos;
      }
      return str_inout;
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

  constexpr Size do_normalize_index(const String &str, Size pos) noexcept
    {
      return (pos == str.npos) ? str.size() : pos;
    }

  template<typename IteratorT>
    constexpr std::pair<std::reverse_iterator<IteratorT>, std::reverse_iterator<IteratorT>> do_reverse_range(std::pair<IteratorT, IteratorT> range)
      {
        return std::make_pair(std::reverse_iterator<IteratorT>(std::move(range.second)), std::reverse_iterator<IteratorT>(std::move(range.first)));
      }

}

Parser_result Token_stream::load(std::istream &sis)
  {
    // Check whether the stream can be read from.
    // For example, we shall fail here if an `std::ifstream` was constructed with a non-existent path.
    Size line = 0;
    if(!sis) {
      return Parser_result(line, 0, 0, Parser_result::error_istream_open_failure);
    }
    // Save the position of an unterminated block comment.
    Size bcom_line = 0;
    Size bcom_off = 0;
    Size bcom_len = 0;
    // Read source code line by line.
    Vector<Token> seq;
    String str;
    while(getline(sis, str)) {
      ++line;
      // Discard the first line if it looks like a shebang.
      if((line == 1) && str.starts_with("#!", 2)) {
        continue;
      }
      ASTERIA_DEBUG_LOG("Parsing line ", std::setw(4), line , ": ", str);
      Size pos, epos;
      ///////////////////////////////////////////////////////////////////////
      // Phase 1
      //   Ensure this line is a valid UTF-8 string.
      ///////////////////////////////////////////////////////////////////////
      pos = 0;
      while(pos < str.size()) {
        // Read the first byte.
        char32_t code = str.at(pos) & 0xFF;
        if(code < 0x80) {
          // This sequence contains only one byte.
          pos += 1;
          continue;
        }
        if(code < 0xC0) {
          // This is not a leading character.
          return Parser_result(line, pos, 1, Parser_result::error_utf8_sequence_invalid);
        }
        if(code >= 0xF8) {
          // If this leading character were valid, it would start a sequence of five bytes or more.
          return Parser_result(line, pos, 1, Parser_result::error_utf8_sequence_invalid);
        }
        // Calculate the number of bytes in this code point.
        const auto u8len = static_cast<unsigned>(2 + (code >= 0xE0) + (code >= 0xF0));
        ROCKET_ASSERT(u8len >= 2);
        ROCKET_ASSERT(u8len <= 4);
        if(str.size() - pos < u8len) {
          // No enough characters have been provided.
          return Parser_result(line, pos, str.size() - pos, Parser_result::error_utf8_sequence_incomplete);
        }
        // Unset bits that are not part of the payload.
        code &= static_cast<unsigned char>(0xFF >> u8len);
        // Accumulate trailing code units.
        for(unsigned i = 1; i < u8len; ++i) {
          const char32_t next = str.at(pos + i) & 0xFF;
          if((next < 0x80) || (0xC0 <= next)) {
            // This trailing character is not valid.
            return Parser_result(line, pos, i + 1, Parser_result::error_utf8_sequence_invalid);
          }
          code = (code << 6) | (next & 0x3F);
        }
        if((0xD800 <= code) && (code < 0xE000)) {
          // Surrogates are not allowed.
          return Parser_result(line, pos, u8len, Parser_result::error_utf_code_point_invalid);
        }
        if(code >= 0x110000) {
          // Code point value is too large.
          return Parser_result(line, pos, u8len, Parser_result::error_utf_code_point_invalid);
        }
        // Re-encode it and check for overlong sequences.
        const auto minlen = static_cast<unsigned>(1 + (code >= 0x80) + (code >= 0x800) + (code >= 0x10000));
        if(minlen != u8len) {
          // Overlong sequences are not allowed.
          return Parser_result(line, pos, u8len, Parser_result::error_utf8_sequence_invalid);
        }
        pos += u8len;
      }
      ///////////////////////////////////////////////////////////////////////
      // Phase 2
      //   Strip comments from this line.
      ///////////////////////////////////////////////////////////////////////
      pos = 0;
      while(pos < str.size()) {
        // Are we inside a block comment?
        if(bcom_line != 0) {
          // Search for the terminator of this block comment.
          static constexpr char s_bcom_term[2] = { '*', '/' };
          epos = str.find(s_bcom_term, pos, 2);
          if(epos == str.npos) {
            // The block comment will not end in this line.
            // Overwrite all characters remaining with spaces.
            do_blank_comment(str, pos, str.size() - pos);
            break;
          }
          // Overwrite this block comment with spaces, including the comment terminator.
          do_blank_comment(str, pos, epos + 2 - pos);
          // Finish this comment.
          bcom_line = 0;
          // Resume from the end of the comment.
          pos = epos + 2;
          continue;
        }
        // Read a character.
        // Skip string literals if any.
        const auto head = str.at(pos);
        if(head == '\'') {
          // Escape sequences do not have special meanings inside single quotation marks.
          epos = str.find('\'', pos);
          if(epos == str.npos) {
            return Parser_result(line, pos, str.size() - pos, Parser_result::error_string_literal_unclosed);
          }
          // Continue from the next character to the end of this string literal.
          pos = epos + 1;
          continue;
        }
        if(head == '\"') {
          // Find the end of the string literal.
          epos = pos + 1;
          for(;;) {
            if(epos >= str.size()) {
              return Parser_result(line, pos, str.size() - pos, Parser_result::error_string_literal_unclosed);
            }
            const auto next = str.at(epos);
            if(next == '\"') {
              // The end of this string is encountered. Finish.
              break;
            }
            if(next != '\\') {
              // Accumulate one character.
              epos += 1;
              continue;
            }
            // Because a backslash cannot occur as part of escape sequences, we just ignore any character escaped for simplicity.
            epos += 2;
          }
          // Continue from the next character to the end of this string literal.
          pos = epos + 1;
          continue;
        }
        if(head == '/') {
          // We have to use `[]` instead of `.at()` here because `pos + 1` may equal `str.size()`.
          const auto next = str[pos + 1];
          if(next == '/') {
            // Start a line comment.
            // Overwrite all characters remaining with spaces.
            do_blank_comment(str, pos, str.size() - pos);
            break;
          }
          if(next == '*') {
            // Start a block comment.
            do_blank_comment(str, pos, 2);
            // Record the beginning of this block.
            ROCKET_ASSERT(line != 0);
            bcom_line = line;
            bcom_off = pos;
            bcom_len = str.size() - pos;
            // Resume from the next character to the initiator.
            pos += 2;
            continue;
          }
        }
        // Skip this character.
        pos += 1;
      }
      ///////////////////////////////////////////////////////////////////////
      // Phase 3
      //   Break this line down into tokens.
      ///////////////////////////////////////////////////////////////////////
      pos = 0;
      while(pos < str.size()) {
        // Skip leading spaces.
        epos = str.find_first_not_of(" \t\v\f\r\n", pos);
        if(epos == str.npos) {
          // This line consists of only spaces. Bail out.
          break;
        }
        pos = epos;
        const auto delims_and_digits = "_:00112233445566778899AaBbCcDdEeFf";
        const auto digits = delims_and_digits + 2;
        // Read a character.
        const auto head = str.at(pos);
        switch(head) {
        case 'A':  case 'B':  case 'C':  case 'D':  case 'E':  case 'F':  case 'G':
        case 'H':  case 'I':  case 'J':  case 'K':  case 'L':  case 'M':  case 'N':
        case 'O':  case 'P':  case 'Q':  case 'R':  case 'S':  case 'T':
        case 'U':  case 'V':  case 'W':  case 'X':  case 'Y':  case 'Z':
        case 'a':  case 'b':  case 'c':  case 'd':  case 'e':  case 'f':  case 'g':
        case 'h':  case 'i':  case 'j':  case 'k':  case 'l':  case 'm':  case 'n':
        case 'o':  case 'p':  case 'q':  case 'r':  case 's':  case 't':
        case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
        case '_':
          {
            // Get an identifier or keyword.
            struct Keyword_element
              {
                char first[14];
                Token::Keyword second;
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
                { "export",    Token::keyword_export    },
                { "false",     Token::keyword_false     },
                { "for",       Token::keyword_for       },
                { "function",  Token::keyword_function  },
                { "if",        Token::keyword_if        },
                { "import",    Token::keyword_import    },
                { "infinity",  Token::keyword_infinity  },
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
            // Get an identifier.
            epos = do_normalize_index(str, str.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_", pos + 1));
            // Check whether this identifier matches a keyword.
            auto range = std::equal_range(std::begin(s_keywords), std::end(s_keywords), head, Prefix_comparator());
            for(;;) {
              if(range.first == range.second) {
                // No matching keyword has been found so far.
                Token::S_identifier token_c = { str.substr(pos, epos - pos) };
                seq.emplace_back(line, pos, epos - pos, std::move(token_c));
                break;
              }
              if((epos - pos == std::strlen(range.first->first)) && (std::memcmp(range.first->first, str.data() + pos, epos - pos) == 0)) {
                // A keyword has been found.
                Token::S_keyword token_c = { range.first->second };
                seq.emplace_back(line, pos, epos - pos, std::move(token_c));
                break;
              }
              ++(range.first);
            }
            break;
          }
        case '!':  case '%':  case '&':  case '(':  case ')':  case '*':  case '+':  case ',':
        case '-':  case '.':  case '/':  case ':':  case ';':  case '<':  case '=':  case '>':
        case '?':  case '[':  case ']':  case '^':  case '{':  case '|':  case '}':  case '~':
          {
            // Get a punctuator.
            struct Punctuator_element
              {
                char first[6];
                Token::Punctuator second;
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
                { "/",     Token::punctuator_div         },
                { "/=",    Token::punctuator_div_eq      },
                { ":",     Token::punctuator_colon       },
                { ";",     Token::punctuator_semicolon   },
                { "<",     Token::punctuator_cmp_lt      },
                { "<<",    Token::punctuator_sla         },
                { "<<<",   Token::punctuator_sll         },
                { "<<<=",  Token::punctuator_sll_eq      },
                { "<<=",   Token::punctuator_sla_eq      },
                { "<=",    Token::punctuator_cmp_lte     },
                { "=",     Token::punctuator_assign      },
                { "==",    Token::punctuator_cmp_eq      },
                { ">",     Token::punctuator_cmp_gt      },
                { ">=",    Token::punctuator_cmp_gte     },
                { ">>",    Token::punctuator_sra         },
                { ">>=",   Token::punctuator_sra_eq      },
                { ">>>",   Token::punctuator_srl         },
                { ">>>=",  Token::punctuator_srl_eq      },
                { "?",     Token::punctuator_condition   },
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
            auto range = do_reverse_range(std::equal_range(std::begin(s_punctuators), std::end(s_punctuators), head, Prefix_comparator()));
            for(;;) {
              if(range.first == range.second) {
                // No matching punctuator has been found so far.
                // This is caused by a `case` value which does not exist in the table above.
                ASTERIA_TERMINATE("The punctuator `", head, "` is unhandled.");
              }
              epos = pos + std::strlen(range.first->first);
              if((epos <= str.size()) && (std::memcmp(range.first->first, str.data() + pos, epos - pos) == 0)) {
                // A punctuator has been found.
                Token::S_punctuator token_c = { range.first->second };
                seq.emplace_back(line, pos, epos - pos, std::move(token_c));
                break;
              }
              ++(range.first);
            }
            break;
          }
        case '\'':
          {
            // Get a string literal.
            epos = str.find('\'', pos + 1);
            if(epos == str.npos) {
              return Parser_result(line, pos, str.size() - pos, Parser_result::error_string_literal_unclosed);
            }
            // Escape sequences do not have special meanings inside single quotation marks.
            // Adjust `epos` to point to the character next to the closing single quotation mark.
            epos += 1;
            // Push the string as-is.
            String value(str, pos + 1, epos - pos - 2);
            Token::S_string_literal token_c = { std::move(value) };
            seq.emplace_back(line, pos, epos - pos, std::move(token_c));
            break;
          }
        case '\"':
          {
            // Get a string literal.
            String value;
            // Escape characters have to be treated specifically inside double quotation marks.
            epos = pos + 1;
            for(;;) {
              if(epos >= str.size()) {
                return Parser_result(line, pos, str.size() - pos, Parser_result::error_string_literal_unclosed);
              }
              auto next = str.at(epos);
              if(next == '\"') {
                // The end of this string is encountered. Finish.
                break;
              }
              if(next != '\\') {
                // This character does not start an escape sequence. Copy it as is.
                value.push_back(next);
                epos += 1;
                continue;
              }
              // Translate an escape sequence.
              unsigned seqlen = 2;
              if(str.size() - epos < seqlen) {
                return Parser_result(line, epos, str.size() - epos, Parser_result::error_escape_sequence_incomplete);
              }
              next = str.at(epos + 1);
              switch(next) {
              case '\'':
                value.push_back('\'');
                break;
              case '\"':
                value.push_back('\"');
                break;
              case '\\':
                value.push_back('\\');
                break;
              case '?':
                value.push_back('?');
                break;
              case 'a':
                value.push_back('\a');
                break;
              case 'b':
                value.push_back('\b');
                break;
              case 'f':
                value.push_back('\f');
                break;
              case 'n':
                value.push_back('\n');
                break;
              case 'r':
                value.push_back('\r');
                break;
              case 't':
                value.push_back('\t');
                break;
              case 'v':
                value.push_back('\v');
                break;
              case '0':
                value.push_back('\x00');
                break;
              case 'Z':
                value.push_back('\x1A');
                break;
              case 'e':
                value.push_back('\x1B');
                break;
              case 'U':
                {
                  seqlen += 2;
                  // Fallthrough.
              case 'u':
                  seqlen += 2;
                  // Fallthrough.
              case 'x':
                  seqlen += 2;
                  // Read hex digits.
                  if(str.size() - epos < seqlen) {
                    return Parser_result(line, epos, str.size() - epos, Parser_result::error_escape_sequence_incomplete);
                  }
                  char32_t code = 0;
                  for(Size i = epos + 2; i < epos + seqlen; ++i) {
                    const auto ptr = std::char_traits<char>::find(digits, 32, str.at(i));
                    if(!ptr) {
                      return Parser_result(line, epos, i, Parser_result::error_escape_sequence_invalid_hex);
                    }
                    const auto digit_value = static_cast<unsigned>((ptr - digits) / 2);
                    code = code * 16 + digit_value;
                  }
                  if((0xD800 <= code) && (code < 0xE000)) {
                    // Surrogates are not allowed.
                    return Parser_result(line, epos, seqlen, Parser_result::error_escape_utf_code_point_invalid);
                  }
                  if(code >= 0x110000) {
                    // Code point value is too large.
                    return Parser_result(line, epos, seqlen, Parser_result::error_escape_utf_code_point_invalid);
                  }
                  // Encode it.
                  const auto encode_one = [&](unsigned shift, unsigned mask)
                    {
                      value.push_back(static_cast<char>((~mask << 1) | ((code >> shift) & mask)));
                    };
                  if(code < 0x80) {
                    encode_one( 0, 0xFF);
                  } else if(code < 0x800) {
                    encode_one( 6, 0x1F);
                    encode_one( 0, 0x3F);
                  } else if(code < 0x10000) {
                    encode_one(12, 0x0F);
                    encode_one( 6, 0x3F);
                    encode_one( 0, 0x3F);
                  } else {
                    encode_one(18, 0x07);
                    encode_one(12, 0x3F);
                    encode_one( 6, 0x3F);
                    encode_one( 0, 0x3F);
                  }
                  break;
                }
              default:
                // Fail if this escape sequence cannot be recognized.
                return Parser_result(line, epos, seqlen, Parser_result::error_escape_sequence_unknown);
              }
              epos += seqlen;
            }
            // Adjust `epos` to point to the character next to the closing double quotation mark.
            epos += 1;
            // Push the string.
            Token::S_string_literal token_c = { std::move(value) };
            seq.emplace_back(line, pos, epos - pos, std::move(token_c));
            break;
          }
        case '0':  case '1':  case '2':  case '3':  case '4':
        case '5':  case '6':  case '7':  case '8':  case '9':
          {
            // Get a numeric literal.
            // Declare everything that will be calculated later.
            // 0. The integral part is required. The fractional and exponent parts are optional.
            // 1. If `frac_begin` equals `int_end` then there is no fractional part.
            // 2. If `exp_begin` equals `frac_end` then there is no exponent part.
            unsigned radix;
            Size int_begin, int_end;
            Size frac_begin, frac_end;
            unsigned exp_base;
            bool exp_sign;
            Size exp_begin, exp_end;
            char next;
            // Check for radix prefixes.
            radix = 10;
            int_begin = pos;
            if(head == '0') {
              // Do not use `str.at()` here as `int_begin + 1` may exceed `str.size()`.
              next = str[int_begin + 1];
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
            int_end = do_normalize_index(str, str.find_first_not_of(delims_and_digits, int_begin, 2 + radix * 2));
            if(int_begin == int_end) {
              return Parser_result(line, pos, int_end - pos, Parser_result::error_numeric_literal_incomplete);
            }
            // Look for the fractional part.
            frac_begin = int_end;
            frac_end = frac_begin;
            next = str[int_end];
            if(next == '.') {
              frac_begin += 1;
              frac_end = do_normalize_index(str, str.find_first_not_of(delims_and_digits, frac_begin, 2 + radix * 2));
              if(frac_begin == frac_end) {
                return Parser_result(line, pos, frac_end - pos, Parser_result::error_numeric_literal_incomplete);
              }
            }
            // Look for the exponent.
            exp_base = 0;
            exp_sign = false;
            exp_begin = frac_end;
            exp_end = exp_begin;
            next = str[frac_end];
            switch(next) {
            case 'E':
            case 'e':
              exp_base = 10;
              exp_begin += 1;
              break;
            case 'P':
            case 'p':
              exp_base = 2;
              exp_begin += 1;
              break;
            }
            if(exp_base != 0) {
              next = str[exp_begin];
              switch(next) {
              case '+':
                exp_sign = false;
                exp_begin += 1;
                break;
              case '-':
                exp_sign = true;
                exp_begin += 1;
                break;
              }
            }
            exp_end = do_normalize_index(str, str.find_first_not_of(delims_and_digits, exp_begin, 22));
            if(exp_begin == exp_end) {
              return Parser_result(line, pos, exp_end - pos, Parser_result::error_numeric_literal_incomplete);
            }
            // Disallow suffixes. Suffixes such as `ll`, `u` and `f` are used in C and C++ to specify the types of numeric literals.
            // Since we make no use of them, we just reserve them for further use for good.
            epos = do_normalize_index(str, str.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.", exp_end));
            if(epos != exp_end) {
              return Parser_result(line, pos, epos - pos, Parser_result::error_numeric_literal_suffix_disallowed);
            }
            // Parse the exponent.
            int exp = 0;
            for(Size i = exp_begin; i != exp_end; ++i) {
              const auto ptr = std::char_traits<char>::find(digits, 20, str.at(i));
              if(!ptr) {
                continue;
              }
              const auto digit_value = static_cast<int>((ptr - digits) / 2);
              const auto bound = (std::numeric_limits<int>::max() - digit_value) / 10;
              if(exp > bound) {
                return Parser_result(line, pos, epos - pos, Parser_result::error_numeric_literal_exponent_overflow);
              }
              exp = exp * 10 + digit_value;
            }
            if(exp_sign) {
              exp = -exp;
            }
            // Is this literal an integral or floating-point number?
            if(frac_begin == int_end) {
              // Parse the literal as an integer.
              // Negative exponents are not allowed, even when the significant part is zero.
              if(exp < 0) {
                return Parser_result(line, pos, epos - pos, Parser_result::error_integer_literal_exponent_negative);
              }
              // Parse the significant part.
              Uint64 value = 0;
              for(Size i = int_begin; i != int_end; ++i) {
                const auto ptr = std::char_traits<char>::find(digits, radix * 2, str.at(i));
                if(!ptr) {
                  continue;
                }
                const auto digit_value = static_cast<Uint64>((ptr - digits) / 2);
                const auto bound = (std::numeric_limits<Uint64>::max() - digit_value) / radix;
                if(value > bound) {
                  return Parser_result(line, pos, epos - pos, Parser_result::error_integer_literal_overflow);
                }
                value = value * radix + digit_value;
              }
              // Raise the significant part to the power of `exp`.
              if(value != 0) {
                for(int i = 0; i < exp; ++i) {
                  const auto bound = std::numeric_limits<Uint64>::max() / exp_base;
                  if(value > bound) {
                    return Parser_result(line, pos, epos - pos, Parser_result::error_integer_literal_overflow);
                  }
                  value *= exp_base;
                }
              }
              // Push an integer literal.
              Token::S_integer_literal token_c = { value };
              seq.emplace_back(line, pos, epos - pos, std::move(token_c));
              break;
            }
            // Parse the literal as a floating-point number.
            // Parse the integral part.
            Xfloat value = 0;
            bool zero = true;
            for(Size i = int_begin; i != int_end; ++i) {
              const auto ptr = std::char_traits<char>::find(digits, radix * 2, str.at(i));
              if(!ptr) {
                continue;
              }
              const auto digit_value = static_cast<int>((ptr - digits) / 2);
              value = value * static_cast<int>(radix) + digit_value;
              zero |= digit_value;
            }
            int vclass = std::fpclassify(value);
            if(vclass == FP_INFINITE) {
              return Parser_result(line, pos, epos - pos, Parser_result::error_real_literal_overflow);
            }
            // Parse the fractional part.
            Xfloat frac = 0;
            for(Size i = frac_end - 1; i + 1 != frac_begin; --i) {
              const auto ptr = std::char_traits<char>::find(digits, radix * 2, str.at(i));
              if(!ptr) {
                continue;
              }
              const auto digit_value = static_cast<int>((ptr - digits) / 2);
              frac = (frac + digit_value) / static_cast<int>(radix);
              zero |= digit_value;
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
              return Parser_result(line, pos, epos - pos, Parser_result::error_real_literal_overflow);
            }
            if((vclass == FP_ZERO) && !zero) {
              return Parser_result(line, pos, epos - pos, Parser_result::error_real_literal_underflow);
            }
            // Push a floating-point literal.
            Token::S_real_literal token_c = { value };
            seq.emplace_back(line, pos, epos - pos, std::move(token_c));
            break;
          }
        default:
          // Fail to find a valid token.
          ASTERIA_DEBUG_LOG("Invalid stray character in source code: ", str.substr(pos, 25));
          return Parser_result(line, pos, 1, Parser_result::error_token_character_unrecognized);
        }
        // Continue from the end of this token.
        pos = epos;
      }
    }
    if(sis.bad()) {
      // If there was an irrecovable I/O failure then any other status code is meaningless.
      return Parser_result(line, 0, 0, Parser_result::error_istream_badbit_set);
    }
    if(bcom_line != 0) {
      // A block comment may straddle multiple lines. We just mark the first line here.
      return Parser_result(bcom_line, bcom_off, bcom_len, Parser_result::error_block_comment_unclosed);
    }
    // Accept the sequence in reverse order.
    this->m_rseq.assign(std::make_move_iterator(seq.mut_rbegin()), std::make_move_iterator(seq.mut_rend()));
    return Parser_result(line, 0, 0, Parser_result::error_success);
  }
void Token_stream::clear() noexcept
  {
    this->m_rseq.clear();
  }

bool Token_stream::empty() const noexcept
  {
    return this->m_rseq.empty();
  }
Size Token_stream::size() const noexcept
  {
    return this->m_rseq.size();
  }
const Token * Token_stream::peek(Size offset) const noexcept
  {
    const auto ntoken = this->m_rseq.size();
    if(offset >= ntoken) {
      return nullptr;
    }
    return this->m_rseq.data() + ntoken - 1 - offset;
  }
Token Token_stream::shift()
  {
    if(this->m_rseq.empty()) {
      ASTERIA_THROW_RUNTIME_ERROR("There are no more tokens from this stream.");
    }
    auto token = std::move(this->m_rseq.mut_back());
    this->m_rseq.pop_back();
    return token;
  }

}
