// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "source_location.hpp"
namespace asteria {

tinyfmt&
Source_Location::
print_to(tinyfmt& fmt) const
  {
    fmt << this->m_file << ':' << this->m_line << ':' << this->m_column;
    return fmt;
  }

}  // namespace asteria
