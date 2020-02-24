// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "parser_error.hpp"
#include "../utilities.hpp"

namespace Asteria {

Parser_Error::~Parser_Error()
  {
  }

void Parser_Error::do_compose_message()
  {
    // Reuse the string.
    ::rocket::tinyfmt_str fmt;
    fmt.set_string(::rocket::move(this->m_what));
    fmt.clear_string();
    // Write the status code in digital form.
    format(fmt, "asteria parser error: ERROR $1: $2", this->m_stat, describe_parser_status(this->m_stat));
    // Append the source location.
    if(this->m_line <= 0)
      fmt << "\n[end of input encountered]";
    else
      format(fmt, "\n[line $1, offset $2, length $3]", this->m_line, this->m_offset, this->m_length);
    // Set the new string.
    this->m_what = fmt.extract_string();
  }

static_assert(::rocket::conjunction<::std::is_nothrow_copy_constructible<Parser_Error>,
                                    ::std::is_nothrow_move_constructible<Parser_Error>,
                                    ::std::is_nothrow_copy_assignable<Parser_Error>,
                                    ::std::is_nothrow_move_assignable<Parser_Error>>::value, "");

}  // namespace Asteria
