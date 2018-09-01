// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PARSER_RESULT_HPP_
#define ASTERIA_PARSER_RESULT_HPP_

#include "fwd.hpp"

namespace Asteria {

class Parser_result
  {
  public:
    enum Error : std::uint32_t
      {
        error_success                              =   0,
        // Category: encoding
        error_utf8_code_unit_invalid               = 101,
        error_utf_code_point_truncated             = 102,
        error_utf_surrogates_disallowed            = 103,
        error_utf_code_point_too_large             = 104,
        error_utf8_encoding_overlong               = 105,
        // Category: tokenizer
        error_token_character_unrecognized         = 201,
        error_string_literal_unclosed              = 202,
        error_escape_sequence_incomplete           = 203,
        error_escape_sequence_unknown              = 204,
        error_escape_sequence_invalid_hex          = 205,
        error_numeric_literal_incomplete           = 206,
        error_numeric_literal_suffixes_disallowed  = 207,
        error_numeric_literal_exponent_overflow    = 208,
        error_integer_literal_overflow             = 209,
        error_integer_literal_exponent_negative    = 210,
        error_double_literal_overflow              = 211,
        error_double_literal_underflow             = 212,
        error_escape_utf_surrogates_disallowed     = 213,
        error_escape_utf_code_point_too_large      = 214,
      };

  private:
    Unsigned m_line;
    Unsigned m_column;
    std::size_t m_length;
    Error m_error;

  public:
    constexpr Parser_result(Unsigned line, Unsigned column, std::size_t length, Error error) noexcept
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
    std::size_t get_length() const noexcept
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
