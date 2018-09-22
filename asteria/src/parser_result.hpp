// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PARSER_RESULT_HPP_
#define ASTERIA_PARSER_RESULT_HPP_

#include "fwd.hpp"

namespace Asteria {

class Parser_result
  {
  public:
    enum Error : Uint32
      {
        error_success                                    =   0,
        // Phase 1
        //   I/O stream
        //   UTF-8 decoder
        error_istream_open_failure                       = 101,
        error_istream_badbit_set                         = 102,
        error_utf8_sequence_invalid                      = 103,
        error_utf8_sequence_incomplete                   = 104,
        error_utf_code_point_invalid                     = 105,
        // Phase 2
        //   Comment stripper
        //   Tokenizer
        error_token_character_unrecognized               = 201,
        error_string_literal_unclosed                    = 202,
        error_escape_sequence_unknown                    = 203,
        error_escape_sequence_incomplete                 = 204,
        error_escape_sequence_invalid_hex                = 205,
        error_escape_utf_code_point_invalid              = 206,
        error_numeric_literal_incomplete                 = 207,
        error_numeric_literal_suffix_disallowed          = 208,
        error_numeric_literal_exponent_overflow          = 209,
        error_integer_literal_overflow                   = 210,
        error_integer_literal_exponent_negative          = 211,
        error_real_literal_overflow                      = 212,
        error_real_literal_underflow                     = 213,
        error_block_comment_unclosed                     = 214,
        // Phase 3
        //   Parser
        error_directive_or_statement_expected            = 301,
        error_identifier_expected                        = 302,
        error_semicolon_expected                         = 303,
        error_string_literal_expected                    = 304,
        error_statement_expected                         = 305,
        error_equals_sign_expected                       = 306,
        error_expression_expected                        = 307,
        error_open_brace_expected                        = 308,
        error_close_brace_or_statement_expected          = 309,
        error_open_parenthesis_expected                  = 310,
        error_close_parenthesis_or_parameter_expected    = 311,
        error_close_parenthesis_expected                 = 312,
        error_colon_expected                             = 313,
        error_close_brace_or_switch_clause_expected      = 314,
        error_keyword_while_expected                     = 315,
        error_keyword_catch_expected                     = 316,
        error_comma_expected                             = 317,
        error_for_statement_initializer_expected         = 318,
        error_close_bracket_or_expression_expected       = 319,
        error_close_brace_or_object_key_expected         = 320,
        error_duplicate_object_key                       = 321,
      };

  public:
    static const char * describe_error(Error error) noexcept;

  private:
    Uint64 m_line;
    Size m_offset;
    Size m_length;
    Error m_error;

  public:
    constexpr Parser_result(Uint64 line, Size offset, Size length, Error error) noexcept
      : m_line(line), m_offset(offset), m_length(length), m_error(error)
      {
      }

  public:
    constexpr Uint64 get_line() const noexcept
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
    constexpr Error get_error() const noexcept
      {
        return this->m_error;
      }
  };

constexpr bool operator==(const Parser_result &lhs, Parser_result::Error rhs) noexcept
  {
    return lhs.get_error() == rhs;
  }
constexpr bool operator!=(const Parser_result &lhs, Parser_result::Error rhs) noexcept
  {
    return lhs.get_error() != rhs;
  }

constexpr bool operator==(Parser_result::Error lhs, const Parser_result &rhs) noexcept
  {
    return lhs == rhs.get_error();
  }
constexpr bool operator!=(Parser_result::Error lhs, const Parser_result &rhs) noexcept
  {
    return lhs != rhs.get_error();
  }

}

#endif
