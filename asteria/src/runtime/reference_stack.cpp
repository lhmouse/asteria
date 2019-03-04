// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_stack.hpp"
#include "../utilities.hpp"

namespace Asteria {

void Reference_Stack::do_switch_to_large()
  {
    ROCKET_ASSERT(this->m_small.size() == this->m_small.capacity());
    ROCKET_ASSERT(this->m_large.capacity() == 0);
    auto tptr = this->m_tptr;
    auto nelem = this->m_small.size();
    // Note that this function has to provide strong exception safety guarantee.
    Cow_Vector<Reference> large;
    large.reserve(nelem * 2);
    for(auto i = nelem; i != 0; --i) {
      large.emplace_back(rocket::move(tptr[-i]));
    }
    tptr = large.mut_data() + nelem;
    // Set up the large buffer.
    this->m_tptr = tptr;
    this->m_large.swap(large);
    this->m_small.clear();
  }

}
