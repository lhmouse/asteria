// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_stack.hpp"
#include "../utils.hpp"

namespace asteria {

void
Reference_Stack::
do_destroy_elements() noexcept
  {
    auto next = this->m_bptr;
    const auto eptr = next + this->m_einit;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qref = next;
      next += 1;

      // Destroy this element.
      ::rocket::destroy_at(qref);
    }
#ifdef ROCKET_DEBUG
    ::std::memset((void*)this->m_bptr, 0xD3, this->m_estor * sizeof(Reference));
#endif
    this->m_etop = 0xDEADBEEF;
  }

void
Reference_Stack::
do_reserve_more()
  {
    // Allocate a new table.
    uint32_t estor = (this->m_estor * 3 / 2 + 5) | 29;
    if(estor <= this->m_estor)
      throw ::std::bad_alloc();

    auto bptr = static_cast<Reference*>(::operator new(estor * sizeof(Reference)));
#ifdef ROCKET_DEBUG
    ::std::memset((void*)bptr, 0xE6, estor * sizeof(Reference));
#endif

    // Move-construct references below `m_etop` into the new storage. This shall
    // not throw exceptions. `m_etop` is left unchanged.
    auto bold = ::std::exchange(this->m_bptr, bptr);
    const uint32_t eiold = ::std::exchange(this->m_einit, this->m_etop);
    this->m_estor = estor;

    if(!bold)
      return;

    // Move references into the new storage.
    for(uint32_t k = 0;  k < eiold;  ++k) {
      // Note again, only references below `m_etop` will be moved.
      if(ROCKET_EXPECT(k < this->m_einit))
        ::rocket::construct_at(bptr + k, ::std::move(bold[k]));

      // Destroy the old reference, always.
      ::rocket::destroy_at(bold + k);
    }
    ::operator delete(bold);
  }

int
Reference_Stack::
do_get_variables_slow(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    auto next = this->m_bptr;
    const auto eptr = next + this->m_einit;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qref = next;
      next += 1;

      // Enumerate variables in this reference.
      qref->get_variables(staged, temp);
    }
    return 0;
  }

}  // namespace asteria
