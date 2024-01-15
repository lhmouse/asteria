// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
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
    this->m_fmt << "compiler error: " << this->m_desc
                << "\n[status " << this->m_stat << " at '" << this->m_sloc << "']";
  }

}  // namespace asteria
