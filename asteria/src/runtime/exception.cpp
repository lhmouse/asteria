// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "exception.hpp"

namespace Asteria {

Exception::~Exception()
  {
  }

const char * Exception::what() const noexcept
  {
    return "Asteria::Exception";
  }

}
