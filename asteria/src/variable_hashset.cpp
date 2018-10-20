// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "variable_hashset.hpp"

namespace Asteria {

Variable_hashset::~Variable_hashset()
  {
    const auto bptr = this->m_data;
    auto eptr = bptr + this->m_nbkt;
    while(eptr != bptr) {
      --eptr;
      rocket::destroy_at(eptr);
    }
    ::operator delete(bptr);
  }

}
