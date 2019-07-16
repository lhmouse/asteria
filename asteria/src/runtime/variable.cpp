// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "variable.hpp"
#include "../utilities.hpp"

namespace Asteria {

Variable::~Variable()
  {
  }

long Variable::increment_gcref(long split) const noexcept
  {
    if(ROCKET_EXPECT(split <= 1)) {
      // Update the integral part only.
      return this->m_gcref.first += 1;
    }
    // Update the fractional part.
    this->m_gcref.second += 1 / static_cast<double>(split);
    // If the result is equal to or greater than one, accumulate the integral part separatedly.
    auto carry = static_cast<long>(this->m_gcref.second);
    this->m_gcref.second -= static_cast<double>(carry);
    return this->m_gcref.first += carry;
  }

void Variable::enumerate_variables(const Abstract_Variable_Callback& callback) const
  {
    this->m_value.enumerate_variables(callback);
  }

}  // namespace Asteria
