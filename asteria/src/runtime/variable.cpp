// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variable.hpp"
#include "../utilities.hpp"

namespace Asteria {

Variable::~Variable()
  {
  }

Variable_Callback& Variable::enumerate_variables(Variable_Callback& callback) const
  {
    return this->m_value.enumerate_variables(callback);
  }

}  // namespace Asteria
