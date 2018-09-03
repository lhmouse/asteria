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
        error_success                              =   0,
        // Category: i/o, encoding
        error_istream_open_failure                 = 101,
        error_istream_fail_or_bad                  = 102,
        error_utf8_sequence_invalid                = 103,
        error_utf8_sequence_truncated              = 104,
        error_utf_code_point_invalid               = 105,
        // Category: tokenizer
        error_token_character_unrecognized         = 201,
        error_string_literal_unclosed              = 202,
        error_escape_sequence_unknown              = 203,
        error_escape_sequence_invalid_hex          = 204,
        error_numeric_literal_incomplete           = 205,
        error_numeric_literal_suffixes_disallowed  = 206,
        error_numeric_literal_exponent_overflow    = 207,
        error_integer_literal_overflow             = 208,
        error_integer_literal_exponent_negative    = 209,
        error_double_literal_overflow              = 210,
        error_double_literal_underflow             = 211,
        error_escape_utf_code_point_invalid        = 212,
      };

  private:
    Unsigned m_line;
    Unsigned m_column;
    Size m_length;
    Error m_error;

  public:
    constexpr Parser_result(Unsigned line, Unsigned column, Size length, Error error) noexcept
      : m_line(line), m_column(column), m_length(length), m_error(error)
      {
      }

  public:
    Unsigned get_line() const noexcept
      {
        return this->m_line;
      }
    Unsigned get_column() const noexcept
      {
        return this->m_column;
      }
    Size get_length() const noexcept
      {
        return this->m_length;
      }
    Error get_error() const noexcept
      {
        return this->m_error;
      }
  };

}

#endif
