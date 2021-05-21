// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "reference_stack.hpp"
#include "../runtime/variable_callback.hpp"
#include "../utils.hpp"

namespace asteria {

void
Reference_Stack::
do_destroy_elements() noexcept
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
    size_t esold = ::std::exchange(this->m_estor, estor);
    this->m_einit = this->m_etop;

    if(!bold)
      return;

    for(size_t k = 0;  k != this->m_einit;  ++k)
      ::rocket::construct_at(bptr + k, ::std::move(bold[k]));

    for(size_t k = 0;  k != esold;  ++k)
      ::rocket::destroy_at(bold + k);

    ::operator delete(bold);
  }

Variable_Callback&
Reference_Stack::
enumerate_variables(Variable_Callback& callback) const
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
