// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Variable::~Variable()
  {
  }
Value & Variable::set_value(Value value, bool immutable)
  {
    if(m_immutable) {
      ASTERIA_THROW_RUNTIME_ERROR("This variable having value `", get_value(), "` is immutable and cannot be modified.");
    }
    m_value = std::move(value);
    m_immutable = immutable;
    return m_value;
  }
bool & Variable::set_immutable(bool immutable) noexcept
  {
    m_immutable = immutable;
    return m_immutable;
  }

}
