// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "reference_stack.hpp"
#include "../utils.hpp"
namespace asteria {

void
Reference_Stack::
do_reallocate(uint32_t estor)
  {
    if(estor != 0) {
      // Extend the storage.
      ROCKET_ASSERT(estor >= this->m_einit);

      if(estor >= 0x7FFF000U / sizeof(Reference))
        throw ::std::bad_alloc();

      auto bptr = (Reference*) ::realloc((void*) this->m_bptr, estor * sizeof(Reference));
      if(!bptr)
        throw ::std::bad_alloc();

      this->m_bptr = bptr;
      this->m_estor = estor;
    }
    else {
      // Free the storage.
      while(this->m_einit != 0)
        ::rocket::destroy(this->m_bptr + -- this->m_einit);

      ::free(this->m_bptr);
      this->m_bptr = nullptr;
      this->m_estor = 0;
    }
  }

}  // namespace asteria
