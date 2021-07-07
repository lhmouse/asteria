// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "compiler_error.hpp"
#include "enums.hpp"

namespace asteria {

static_assert(
    ::std::is_nothrow_copy_constructible<Compiler_Error>::value &&
    ::std::is_nothrow_move_constructible<Compiler_Error>::value &&
    ::std::is_nothrow_copy_assignable<Compiler_Error>::value &&
    ::std::is_nothrow_move_assignable<Compiler_Error>::value);

Compiler_Error::
~Compiler_Error()
  {
  }

void
Compiler_Error::
do_compose_message()
  {
    // Reuse the storage if any.
    ::rocket::tinyfmt_str fmt;
    fmt.set_string(::std::move(this->m_what));
    fmt.clear_string();

    // Write the status code in digital form.
    format(fmt,
        "compiler error: $1\n[status $2 at '$3']",
        describe_compiler_status(this->m_stat), this->m_stat, this->m_sloc);

    if(this->m_unm_sloc.line() > 0)
      format(fmt,
          "\n[unmatched `$1` at '$2']",
          stringify_punctuator(this->m_unm_punct), this->m_unm_sloc);

    // Set the new string.
    this->m_what = fmt.extract_string();
  }

}  // namespace asteria
