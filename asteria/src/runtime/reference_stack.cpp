// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_stack.hpp"
#include "../utilities.hpp"

namespace Asteria {

Reference_stack::~Reference_stack()
  {
  }

void Reference_stack::do_switch_to_large()
  {
    ROCKET_ASSERT(this->m_small.size() == this->m_small.capacity());
    ROCKET_ASSERT(this->m_large.capacity() == 0);
    auto tptr = this->m_tptr;
    const auto nelem = this->m_small.size();
    // Note that this function has to provide strong exception safety guarantee.
    rocket::cow_vector<Reference> large;
    large.reserve(nelem * 2);
    for(auto i = nelem; i != 0; --i) {
      large.emplace_back(std::move(tptr[-i]));
    }
    tptr = large.mut_data() + nelem;
    // Set up the large buffer.
    this->m_tptr = tptr;
    this->m_large.swap(large);
    this->m_small.clear();
  }

}
