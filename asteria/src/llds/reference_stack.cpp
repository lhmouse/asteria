// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_stack.hpp"
#include "../runtime/variable_callback.hpp"
#include "../util.hpp"

namespace asteria {

void
Reference_Stack::
do_destroy_elements()
  noexcept
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_einit;
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
do_reallocate_reserve(Reference*& bptr, uint32_t& estor, uint32_t nadd)
  const
  {
    // Allocate a new table.
    constexpr size_t size_max = PTRDIFF_MAX / sizeof(Reference);
    size_t size_cur = this->size();
    if(size_max - size_cur < nadd)
      throw ::std::bad_array_new_length();

    estor = static_cast<uint32_t>(size_cur + nadd);
    bptr = static_cast<Reference*>(::operator new(estor * sizeof(Reference)));
  }

void
Reference_Stack::
do_reallocate_finish(Reference* bptr, uint32_t estor)
  noexcept
  {
    // Set up the partially initialized storage.
    // The caller may have constructed elements above the top, but the top index
    // shall be left intact.
    auto bold = ::std::exchange(this->m_bptr, bptr);
    uint32_t etop = this->m_etop;
    uint32_t eiold = ::std::exchange(this->m_einit, etop);
    this->m_estor = estor;

    // Relocate references within [0,top]. Destroy the others.
    for(uint32_t k = 0;  k != etop;  ++k) {
      ::rocket::construct_at(bptr + k, ::std::move(bold[k]));
      ::rocket::destroy_at(bold + k);
    }
    for(uint32_t k = etop;  k != eiold;  ++k) {
      ::rocket::destroy_at(bold + k);
    }
    if(bold)
      ::operator delete(bold);
  }

Variable_Callback&
Reference_Stack::
enumerate_variables(Variable_Callback& callback)
  const
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_einit;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qref = next;
      next += 1;

      // Enumerate variables in this reference.
      qref->enumerate_variables(callback);
    }
    return callback;
  }

}  // namespace asteria
