// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_STACK_HPP_
#define ASTERIA_LLDS_REFERENCE_STACK_HPP_

#include "../fwd.hpp"
#include "../runtime/reference.hpp"

namespace asteria {

class Reference_Stack
  {
  private:
    Reference* m_bptr = nullptr;  // beginning of raw storage
    uint32_t m_etop = 0;   // offset past the top (must be within [0,einit])
    uint32_t m_einit = 0;  // offset to the last initialized reference
    uint32_t m_estor = 0;  // end of reserved storage

  public:
    explicit constexpr
    Reference_Stack() noexcept
      { }

    Reference_Stack(Reference_Stack&& other) noexcept
      { this->swap(other);  }

    Reference_Stack&
    operator=(Reference_Stack&& other) noexcept
      { return this->swap(other);  }

    Reference_Stack&
    swap(Reference_Stack& other) noexcept
      { ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_etop, other.m_etop);
        ::std::swap(this->m_einit, other.m_einit);
        ::std::swap(this->m_estor, other.m_estor);
        return *this;  }

  private:
    void
    do_destroy_elements(bool xfree) noexcept;

    void
    do_reserve_more(uint32_t nadd);

  public:
    ~Reference_Stack()
      {
        if(this->m_bptr)
          this->do_destroy_elements(true);
      }

    bool
    empty() const noexcept
      { return this->m_etop == 0;  }

    size_t
    size() const noexcept
      { return this->m_etop;  }

    Reference_Stack&
    clear() noexcept
      {
        this->m_etop = 0;
        return *this;
      }

    void
    get_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
      {
        auto bptr = this->m_bptr;
        auto eptr = this->m_bptr + this->m_einit;
        while(eptr != bptr)
          (--eptr)->get_variables(staged, temp);
      }

    Reference_Stack&
    clear_cache() noexcept
      {
        auto tptr = this->m_bptr + this->m_etop;
        auto eptr = this->m_bptr + this->m_einit;
        while(eptr != tptr)
          ::rocket::destroy(--eptr);

        this->m_einit = this->m_etop;
        return *this;
      }

    const Reference&
    top(size_t index = 0) const noexcept
      {
        size_t ki = this->m_etop + ~index;
        ROCKET_ASSERT(ki < this->m_etop);
        return this->m_bptr[ki];
      }

    Reference&
    mut_top(size_t index = 0)
      {
        size_t ki = this->m_etop + ~index;
        ROCKET_ASSERT(ki < this->m_etop);
        return this->m_bptr[ki];
      }

    ROCKET_ALWAYS_INLINE Reference&
    push()
      {
        size_t ki = this->m_etop;
        if(ROCKET_UNEXPECT(ki >= this->m_einit)) {
          // Construct one more reference.
          if(ROCKET_UNEXPECT(ki >= this->m_estor))
            this->do_reserve_more(7);

          ROCKET_ASSERT(ki < this->m_estor);
          ::rocket::construct(this->m_bptr + ki);
          this->m_einit += 1;
        }
        this->m_etop += 1;
        return this->m_bptr[ki];
      }

    Reference_Stack&
    pop(size_t count = 1) noexcept
      {
        ROCKET_ASSERT(count <= this->m_etop);
        this->m_etop -= (uint32_t)count;
        return *this;
      }
  };

inline void
swap(Reference_Stack& lhs, Reference_Stack& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
