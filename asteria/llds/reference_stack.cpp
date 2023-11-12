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
    if(estor >= 0x7FFE0000U / sizeof(Reference))
      throw ::std::bad_alloc();

    ROCKET_ASSERT(estor >= this->m_einit);
    auto bptr = (Reference*) ::realloc((void*) this->m_bptr, estor * sizeof(Reference));
    if(!bptr)
      throw ::std::bad_alloc();

#ifdef ROCKET_DEBUG
    ::memset((void*) (bptr + this->m_einit), 0xC3, (estor - this->m_einit) * sizeof(Reference));
#endif

    this->m_bptr = bptr;
    this->m_estor = estor;
  }

void
Reference_Stack::
do_deallocate() noexcept
  {
    this->clear();
    this->clear_red_zone();

#ifdef ROCKET_DEBUG
    ::memset((void*) this->m_bptr, 0xD9, this->m_estor * sizeof(Reference));
#endif
    ::free(this->m_bptr);

    this->m_bptr = nullptr;
    this->m_estor = 0;
  }

void
Reference_Stack::
clear_red_zone() noexcept
  {
    while(this->m_einit != this->m_etop)
      ::rocket::destroy(this->m_bptr + (-- this->m_einit));
  }

void
Reference_Stack::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    for(uint32_t refi = 0;  refi != this->m_einit;  ++ refi)
      this->m_bptr[refi].collect_variables(staged, temp);
  }

}  // namespace asteria
