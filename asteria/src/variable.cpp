// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Variable::~Variable()
  {
  }

[[noreturn]] void Variable::do_throw_immutable() const
  {
    ASTERIA_THROW_RUNTIME_ERROR("This variable having value `", this->m_value, "` is immutable and cannot be modified.");
  }

}
