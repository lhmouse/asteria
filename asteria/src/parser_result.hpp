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
        error_success                            =   0,
        // Category: i/o, encoding
        error_istream_open_failure               = 101,
        error_istream_badbit_set                 = 102,
        error_utf8_sequence_invalid              = 103,
        error_utf8_sequence_incomplete           = 104,
        error_utf_code_point_invalid             = 105,
        // Category: tokenizer
        error_token_character_unrecognized       = 201,
        error_string_literal_unclosed            = 202,
        error_escape_sequence_unknown            = 203,
        error_escape_sequence_incomplete         = 204,
        error_escape_sequence_invalid_hex        = 205,
        error_escape_utf_code_point_invalid      = 206,
        error_numeric_literal_incomplete         = 207,
        error_numeric_literal_suffix_disallowed  = 208,
        error_numeric_literal_exponent_overflow  = 209,
        error_integer_literal_overflow           = 210,
        error_integer_literal_exponent_negative  = 211,
        error_double_literal_overflow            = 212,
        error_double_literal_underflow           = 213,
        error_block_comment_unclosed             = 214,
      };

  private:
    Unsigned m_line;
    Size m_offset;
    Size m_length;
    Error m_error;

  public:
    constexpr Parser_result(Unsigned line, Size offset, Size length, Error error) noexcept
      : m_line(line), m_offset(offset), m_length(length), m_error(error)
      {
      }

  public:
    Unsigned get_line() const noexcept
      {
        return this->m_line;
      }
    Size get_offset() const noexcept
      {
        return this->m_offset;
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
