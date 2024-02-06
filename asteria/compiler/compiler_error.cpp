// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "compiler_error.hpp"
#include "enums.hpp"
namespace asteria {

Compiler_Error::
~Compiler_Error()
  {
  }

void
Compiler_Error::
do_compose_message()
  {
    // Rebuild the message. The storage may be reused.
    this->m_fmt.clear_string();
    format(this->m_fmt, "compiler error: $1", this->m_desc);
    format(this->m_fmt, "\n[status $1 at '$2']", this->m_status, this->m_sloc);
  }

}  // namespace asteria
