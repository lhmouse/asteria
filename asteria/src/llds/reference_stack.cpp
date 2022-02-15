// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_stack.hpp"
#include "../utils.hpp"

namespace asteria {

void
Reference_Stack::
do_destroy_elements(bool xfree) noexcept
  {
    auto next = this->m_bptr;
    const auto eptr = next + this->m_einit;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qref = next;
      next += 1;

      // Destroy this element.
      ::rocket::destroy(qref);
    }

    this->m_etop = 0xDEADBEEF;
    if(!xfree)
      return;

    // Deallocate the old table.
    auto bold = ::std::exchange(this->m_bptr, (Reference*)0xDEADBEEF);
    auto esold = ::std::exchange(this->m_estor, (size_t)0xBEEFDEAD);
    ::rocket::freeN<Reference>(bold, esold);
  }

void
Reference_Stack::
do_reserve_more(uint32_t nadd)
  {
    // Allocate a new table.
    uint32_t estor = (this->m_estor * 3 / 2 + nadd + 5) | 17;
    if(estor <= this->m_estor)
      throw ::std::bad_alloc();

    auto bptr = ::rocket::allocN<Reference>(estor);
#ifdef ROCKET_DEBUG
    ::std::memset((void*)bptr, 0xE6, estor * sizeof(Reference));
#endif

    // Move-construct references below `m_etop` into the new storage. This shall
    // not throw exceptions. `m_etop` is left unchanged.
    auto bold = ::std::exchange(this->m_bptr, bptr);
    auto eiold = ::std::exchange(this->m_einit, this->m_etop);
    auto esold = ::std::exchange(this->m_estor, estor);
    if(ROCKET_EXPECT(!bold))
      return;

    // Move references into the new storage.
    for(uint32_t k = 0;  k < eiold;  ++k) {
      // Note again, only references below `m_etop` will be moved.
      if(ROCKET_EXPECT(k < this->m_einit))
        ::rocket::construct(bptr + k, ::std::move(bold[k]));

      // Destroy the old reference, always.
      ::rocket::destroy(bold + k);
    }

    // Deallocate the old table.
    ::rocket::freeN<Reference>(bold, esold);
  }

}  // namespace asteria
