// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PARSER_ERROR_HPP_
#define ASTERIA_PARSER_ERROR_HPP_

#include "fwd.hpp"

namespace Asteria {

class Parser_error
  {
  public:
    enum Code : Uint32
      {
        // Special
        code_success                                    =   0,
        code_no_data_loaded                             =   1,
        // Phase 1
        //   I/O stream
        //   UTF-8 decoder
        code_istream_open_failure                       = 101,
        code_istream_badbit_set                         = 102,
        code_utf8_sequence_invalid                      = 103,
        code_utf8_sequence_incomplete                   = 104,
        code_utf_code_point_invalid                     = 105,
        // Phase 2
        //   Tokenizer
        code_token_character_unrecognized               = 201,
        code_string_literal_unclosed                    = 202,
        code_escape_sequence_unknown                    = 203,
        code_escape_sequence_incomplete                 = 204,
        code_escape_sequence_invalid_hex                = 205,
        code_escape_utf_code_point_invalid              = 206,
        code_numeric_literal_incomplete                 = 207,
        code_numeric_literal_suffix_disallowed          = 208,
        code_numeric_literal_exponent_overflow          = 209,
        code_integer_literal_overflow                   = 210,
        code_integer_literal_exponent_negative          = 211,
        code_real_literal_overflow                      = 212,
        code_real_literal_underflow                     = 213,
        code_block_comment_unclosed                     = 214,
        // Phase 3
        //   Parser
        code_identifier_expected                        = 302,
        code_semicolon_expected                         = 303,
        code_string_literal_expected                    = 304,
        code_statement_expected                         = 305,
        code_equals_sign_expected                       = 306,
        code_expression_expected                        = 307,
        code_open_brace_expected                        = 308,
        code_close_brace_or_statement_expected          = 309,
        code_open_parenthesis_expected                  = 310,
        code_close_parenthesis_or_parameter_expected    = 311,
        code_close_parenthesis_expected                 = 312,
        code_colon_expected                             = 313,
        code_close_brace_or_switch_clause_expected      = 314,
        code_keyword_while_expected                     = 315,
        code_keyword_catch_expected                     = 316,
        code_comma_expected                             = 317,
        code_for_statement_initializer_expected         = 318,
        code_close_bracket_or_expression_expected       = 319,
        code_close_brace_or_object_key_expected         = 320,
        code_duplicate_object_key                       = 321,
        code_close_parenthesis_or_argument_expected     = 322,
        code_close_bracket_expected                     = 323,
      };

  public:
    static const char * get_code_description(Code code) noexcept;

  private:
    Uint32 m_line;
    Size m_offset;
    Size m_length;
    Code m_code;

  public:
    constexpr Parser_error(Uint32 line, Size offset, Size xlength, Code code) noexcept
      : m_line(line), m_offset(offset), m_length(xlength), m_code(code)
      {
      }

  public:
    constexpr Uint32 get_line() const noexcept
      {
        return this->m_line;
      }
    constexpr Size get_offset() const noexcept
      {
        return this->m_offset;
      }
    constexpr Size get_length() const noexcept
      {
        return this->m_length;
      }
    constexpr Code get_code() const noexcept
      {
        return this->m_code;
      }
  };

constexpr bool operator==(const Parser_error &lhs, Parser_error::Code rhs) noexcept
  {
    return lhs.get_code() == rhs;
  }
constexpr bool operator!=(const Parser_error &lhs, Parser_error::Code rhs) noexcept
  {
    return lhs.get_code() != rhs;
  }

constexpr bool operator==(Parser_error::Code lhs, const Parser_error &rhs) noexcept
  {
    return lhs == rhs.get_code();
  }
constexpr bool operator!=(Parser_error::Code lhs, const Parser_error &rhs) noexcept
  {
    return lhs != rhs.get_code();
  }

}

#endif
