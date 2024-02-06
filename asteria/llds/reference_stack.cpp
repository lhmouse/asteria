// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "reference_stack.hpp"
#include "../utils.hpp"
namespace asteria {

ROCKET_FLATTEN
void
Reference_Stack::
do_reallocate(uint32_t estor)
  {
    if(estor >= 0x7FFE0000U / sizeof(Reference))
      throw ::std::bad_alloc();

    ROCKET_ASSERT(estor >= this->m_einit);
    ::rocket::xmeminfo minfo;
    minfo.element_size = sizeof(Reference);
    minfo.count = estor;
    ::rocket::xmemalloc(minfo);

    if(this->m_bptr) {
      ::rocket::xmeminfo rinfo;
      rinfo.element_size = sizeof(Reference);
      rinfo.data = this->m_bptr;
      rinfo.count = this->m_einit;
      ::rocket::xmemcopy(minfo, rinfo);

      rinfo.count = this->m_estor;
      ::rocket::xmemfree(rinfo);
    }

    this->m_bptr = (Reference*) minfo.data;
    this->m_estor = (uint32_t) minfo.count;
  }

ROCKET_FLATTEN
void
Reference_Stack::
do_deallocate() noexcept
  {
    this->clear();
    this->clear_red_zone();

    ::rocket::xmeminfo rinfo;
    rinfo.element_size = sizeof(Reference);
    rinfo.data = this->m_bptr;
    rinfo.count = this->m_estor;
    ::rocket::xmemfree(rinfo);

    this->m_bptr = nullptr;
    this->m_estor = 0;
  }

void
Reference_Stack::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    for(uint32_t k = 0;  k != this->m_einit;  ++ k)
      this->m_bptr[k].collect_variables(staged, temp);
  }

}  // namespace asteria
