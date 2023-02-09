// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "precompiled.ipp"
#include "source_location.hpp"
namespace asteria {

tinyfmt&
Source_Location::
print(tinyfmt& fmt) const
  {
    return fmt << this->m_file << ':' << this->m_line << ':' << this->m_column;
  }

}  // namespace asteria
