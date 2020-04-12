// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "parser_error.hpp"
#include "../utilities.hpp"

namespace Asteria {

static_assert(::std::is_nothrow_copy_constructible<Parser_Error>::value &&
              ::std::is_nothrow_move_constructible<Parser_Error>::value &&
              ::std::is_nothrow_copy_assignable<Parser_Error>::value &&
              ::std::is_nothrow_move_assignable<Parser_Error>::value);

Parser_Error::
~Parser_Error()
  {
  }

void
Parser_Error::
do_compose_message()
  {
    // Reuse the string.
    ::rocket::tinyfmt_str fmt;
    fmt.set_string(::std::move(this->m_what));
    fmt.clear_string();

    // Write the status code in digital form.
    format(fmt, "asteria parser error $1: $2\n"
                "[unexpected token at '$3']",
                this->m_stat, describe_parser_status(this->m_stat),
                this->m_sloc);

    // Set the new string.
    this->m_what = fmt.extract_string();
  }

}  // namespace Asteria
