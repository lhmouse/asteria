// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "source_location.hpp"

namespace Asteria {

tinyfmt& Source_Location::print(tinyfmt& fmt) const
  {
    return fmt << this->m_file << ':' << this->m_line;
  }

}  // namespace Asteria
