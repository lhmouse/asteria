// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "source_location.hpp"

namespace Asteria {

std::ostream & operator<<(std::ostream &os, const Source_location &loc)
  {
    os << loc.get_file() << ':' << loc.get_line();
    return os;
  }

}
