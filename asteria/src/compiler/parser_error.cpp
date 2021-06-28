// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "parser_error.hpp"
#include "enums.hpp"

namespace asteria {

static_assert(
    ::std::is_nothrow_copy_constructible<Parser_Error>::value &&
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
    // Reuse the storage if any.
    ::rocket::tinyfmt_str fmt;
    fmt.set_string(::std::move(this->m_what));
    fmt.clear_string();

    // Write the status code in digital form.
    format(fmt,
        "parser error [$1]: $2\n[near '$3']",
        this->m_stat, describe_parser_status(this->m_stat), this->m_sloc);

    if(this->m_unm_sloc.line() > 0)
      format(fmt,
          "\n[unmatched `$1` at '$2']",
          stringify_punctuator(this->m_unm_punct), this->m_unm_sloc);

    // Set the new string.
    this->m_what = fmt.extract_string();
  }

}  // namespace asteria
