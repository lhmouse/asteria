// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "compiler_error.hpp"
#include "enums.hpp"
namespace asteria {
namespace {

static_assert(::std::is_nothrow_copy_constructible<Compiler_Error>::value);
static_assert(::std::is_nothrow_move_constructible<Compiler_Error>::value);
static_assert(::std::is_nothrow_copy_assignable<Compiler_Error>::value);
static_assert(::std::is_nothrow_move_assignable<Compiler_Error>::value);

}  // namespace

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
