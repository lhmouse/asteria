// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "source_location.hpp"
namespace asteria {

tinyfmt&
Source_Location::
print_to(tinyfmt& fmt) const
  {
    return fmt << this->m_file << ':' << this->m_line << ':' << this->m_column;
  }

}  // namespace asteria
