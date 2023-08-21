// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "reference_stack.hpp"
#include "../utils.hpp"
namespace asteria {

void
Reference_Stack::
do_deallocate() noexcept
  {
    while(this->m_einit != 0)
      ::rocket::destroy(this->m_bptr + -- this->m_einit);

    ::rocket::freeN<Reference>(this->m_bptr, this->m_estor);
    this->m_bptr = nullptr;
  }

void
Reference_Stack::
do_reserve_more()
  {
    // Allocate a new table.
    uint32_t estor = (this->m_estor * 3 / 2 + 5) | 17;
    if(estor <= this->m_estor)
      throw ::std::bad_alloc();

    auto bptr = ::rocket::allocN<Reference>(estor);
#ifdef ROCKET_DEBUG
    ::std::memset((void*) bptr, 0xE6, estor * sizeof(Reference));
#endif

    // Move-construct references below `m_etop` into the new storage. This shall
    // not throw exceptions. `m_etop` is left unchanged.
    auto bold = ::std::exchange(this->m_bptr, bptr);
    auto eiold = ::std::exchange(this->m_einit, this->m_etop);
    auto esold = ::std::exchange(this->m_estor, estor);

    if(ROCKET_EXPECT(!bold))
      return;

    for(uint32_t k = 0;  k < eiold;  ++k) {
      // Only references below `m_etop` will be moved.
      if(k < this->m_einit)
        ::rocket::construct(bptr + k, ::std::move(bold[k]));

      // Destroy the old reference, always.
      ::rocket::destroy(bold + k);
    }

#ifdef ROCKET_DEBUG
    ::std::memset((void*) bold, 0xD9, esold * sizeof(Reference));
#endif
    ::rocket::freeN<Reference>(bold, esold);
  }

}  // namespace asteria
