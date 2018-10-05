// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable.hpp"
#include "utilities.hpp"

namespace Asteria {

Variable::~Variable()
  {
  }

void Variable::set_value(Value value)
  {
    if(this->m_immutable) {
      ASTERIA_THROW_RUNTIME_ERROR("This variable having value `", get_value(), "` is immutable and cannot be modified.");
    }
    this->m_value = std::move(value);
  }

void Variable::reset(Value value, bool immutable)
  {
    this->m_value = std::move(value);
    this->m_immutable = immutable;
  }

}
