// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PARSER_RESULT_HPP_
#define ASTERIA_PARSER_RESULT_HPP_

#include "fwd.hpp"

namespace Asteria
{

class Parser_result
  {
  public:
    enum Error_code : std::uint32_t
      {
        error_code_success                              =   0,
        // Category: encoding
        error_code_utf8_code_unit_invalid               = 101,
        error_code_utf8_code_point_truncated            = 102,
        error_code_utf8_surrogates_disallowed           = 103,
        error_code_utf_code_point_value_too_large       = 104,
        error_code_utf8_encoding_overlong               = 105,
        // Category: tokenizer
        error_code_token_character_unrecognized         = 201,
        error_code_string_literal_unclosed              = 202,
        error_code_escape_sequence_incomplete           = 203,
        error_code_escape_sequence_unknown              = 204,
        error_code_escape_sequence_invalid_hex          = 205,
        error_code_numeric_literal_incomplete           = 206,
        error_code_numeric_literal_suffixes_disallowed  = 207,
        error_code_numeric_literal_exponent_overflow    = 208,
        error_code_integer_literal_overflow             = 209,
        error_code_integer_literal_exponent_negative    = 210,
        error_code_double_literal_overflow              = 211,
        error_code_double_literal_underflow             = 212,
      };

  private:
    std::size_t m_line;
    std::size_t m_column;
    std::size_t m_length;
    Error_code m_error_code;

  public:
    constexpr Parser_result(std::size_t line, std::size_t column, std::size_t length, Error_code error_code) noexcept
      : m_line(line), m_column(column), m_length(length), m_error_code(error_code)
      {
      }

  public:
    std::size_t get_line() const noexcept
      {
        return m_line;
      }
    std::size_t get_column() const noexcept
      {
        return m_column;
      }
    std::size_t get_length() const noexcept
      {
        return m_length;
      }
    Error_code get_error_code() const noexcept
      {
        return m_error_code;
      }

  public:
    explicit operator bool () const noexcept
      {
        return m_error_code == error_code_success;
      }
  };

}

#endif
