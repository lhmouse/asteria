// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

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

    ROCKET_ASSERT(estor >= this->m_size);
    ::rocket::xmeminfo minfo;
    minfo.element_size = sizeof(Reference);
    minfo.count = estor;
    ::rocket::xmemalloc(minfo);

    if(this->m_bptr) {
      // Move old references into the new stack.
      ::rocket::xmeminfo rinfo;
      rinfo.element_size = sizeof(Reference);
      rinfo.data = this->m_bptr;
      rinfo.count = this->m_size;
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
do_clear(bool free_storage)
  noexcept
  {
    while(this->m_size != 0) {
      this->m_size --;
      ::rocket::destroy(this->m_bptr + this->m_size);
    }

    if(free_storage) {
      ::rocket::xmeminfo rinfo;
      rinfo.element_size = sizeof(Reference);
      rinfo.data = this->m_bptr;
      rinfo.count = this->m_estor;
      ::rocket::xmemfree(rinfo);

      this->m_bptr = nullptr;
      this->m_estor = 0;
    }

    ROCKET_ASSERT(this->m_size == 0);
  }

}  // namespace asteria
