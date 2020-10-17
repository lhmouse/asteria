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
    auto next = this->m_bptr + this->m_bstor;
    const auto eptr = this->m_bptr + this->m_estor;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qref = ::std::exchange(next, next + 1);

      // Destroy this element.
      ::rocket::destroy_at(qref);
    }
#ifdef ROCKET_DEBUG
    ::std::memset((void*)this->m_bptr, 0xD3, this->m_estor * sizeof(Reference));
#endif
    this->m_top = 0xDEADBEEF;
  }

void
Reference_Stack::
do_reallocate_reserve(Reference*& bptr, uint32_t& estor, uint32_t nadd)
  const
  {
    // Allocate a new table.
    constexpr size_t size_max = PTRDIFF_MAX / sizeof(Reference);
    uint32_t size_cur = this->m_estor - this->m_top;
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
    // Calculate the new top offset.
    // The caller may have constructed elements above the top.
    uint32_t bstor = estor - (this->m_estor - this->m_top);

    // Set up the partially initialized storage.
    auto bold = ::std::exchange(this->m_bptr, bptr);
    uint32_t bsold = ::std::exchange(this->m_bstor, bstor);
    uint32_t told = ::std::exchange(this->m_top, bstor);
    uint32_t esold = ::std::exchange(this->m_estor, estor);

    // Destroy elements above the old top and relocate the others.
    for(uint32_t k = bsold;  k != told;  ++k) {
      ::rocket::destroy_at(bold + k);
    }
    for(uint32_t k = told;  k != esold;  ++k) {
      ::rocket::construct_at(bptr + bstor - told + k, ::std::move(bold[k]));
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
    auto next = this->m_bptr + this->m_bstor;
    const auto eptr = this->m_bptr + this->m_estor;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qref = ::std::exchange(next, next + 1);

      // Enumerate variables in this reference.
      qref->enumerate_variables(callback);
    }
    return callback;
  }

}  // namespace asteria
