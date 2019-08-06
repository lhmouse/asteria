// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "source_location.hpp"

namespace Asteria {

std::ostream& Source_Location::print(std::ostream& ostrm) const
  {
    return ostrm << this->m_file << ':' << this->m_line;
  }

}  // namespace Asteria
