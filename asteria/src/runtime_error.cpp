// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "runtime_error.hpp"

namespace Asteria {

Runtime_error::~Runtime_error()
  {
  }

const char * Runtime_error::what() const noexcept
  {
    return this->m_msg.c_str();
  }

}
