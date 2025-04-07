// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_STACK_
#define ASTERIA_LLDS_REFERENCE_STACK_

#include "../fwd.hpp"
#include "../runtime/reference.hpp"
namespace asteria {

class Reference_Stack
  {
  private:
    Reference* m_bptr = nullptr;
    uint32_t m_etop = 0;
    uint32_t m_einit = 0;
    uint32_t m_estor = 0;

  public:
    constexpr Reference_Stack() noexcept = default;

    Reference_Stack(Reference_Stack&& other) noexcept
      {
        this->swap(other);
      }

    Reference_Stack&
    operator=(Reference_Stack&& other) & noexcept
      {
        this->swap(other);
        return *this;
      }

    Reference_Stack&
    swap(Reference_Stack& other) noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_etop, other.m_etop);
        ::std::swap(this->m_einit, other.m_einit);
        ::std::swap(this->m_estor, other.m_estor);
        return *this;
      }

  private:
    void
    do_reallocate(uint32_t estor);

    void
    do_deallocate() noexcept;

  public:
    ~Reference_Stack()
      {
        if(this->m_bptr)
          this->do_deallocate();
      }

    bool
    empty() const noexcept
      { return this->m_etop == 0;  }

    uint32_t
    size() const noexcept
      { return this->m_etop;  }

    void
    clear() noexcept
      {
        this->m_etop = 0;
      }

    void
    clear_red_zone() noexcept
      {
        while(this->m_einit != this->m_etop) {
          this->m_einit --;
          ::rocket::destroy(this->m_bptr + this->m_einit);
        }
      }

    const Reference&
    top(uint32_t index = 0) const noexcept
      {
        ROCKET_ASSERT(index < this->m_etop);
        return *(this->m_bptr + this->m_etop - 1 - index);
      }

    Reference&
    mut_top(uint32_t index = 0)
      {
        ROCKET_ASSERT(index < this->m_etop);
        return *(this->m_bptr + this->m_etop - 1 - index);
      }

    const Reference&
    bottom(uint32_t index = 0) const noexcept
      {
        ROCKET_ASSERT(index < this->m_etop);
        return *(this->m_bptr + index);
      }

    Reference&
    mut_bottom(uint32_t index = 0)
      {
        ROCKET_ASSERT(index < this->m_etop);
        return *(this->m_bptr + index);
      }

    Reference&
    push()
      {
#ifdef ROCKET_DEBUG
        this->do_reallocate(this->m_einit + 1);
#endif  // debug

        if(ROCKET_UNEXPECT(this->m_etop >= this->m_einit)) {
          if(this->m_einit >= this->m_estor)
            this->do_reallocate(this->m_estor / 2 * 3 | 17);

          // Construct a new reference.
          ::rocket::construct(this->m_bptr + this->m_einit);
          this->m_einit ++;
        }

        this->m_etop ++;
        return *(this->m_bptr + this->m_etop - 1);
      }

    void
    pop(uint32_t count = 1) noexcept
      {
        ROCKET_ASSERT(count <= this->m_etop);
        this->m_etop -= count;
      }

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const;
  };

inline
void
swap(Reference_Stack& lhs, Reference_Stack& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
