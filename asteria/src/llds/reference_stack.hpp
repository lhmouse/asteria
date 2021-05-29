// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

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

  private:
    void
    do_destroy_elements() noexcept;

    void
    do_reserve_more();

    void
    do_move_storage_from(Reference* bold, uint32_t esold) noexcept;

  public:
    ~Reference_Stack()
      {
        if(this->m_einit)
          this->do_destroy_elements();

        if(this->m_bptr)
          ::operator delete(this->m_bptr);

#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this), 0xBA, sizeof(*this));
#endif
      }

    bool
    empty() const noexcept
      { return this->m_etop == 0;  }

    size_t
    size() const noexcept
      { return this->m_etop;  }

    const Reference*
    bottom() const noexcept
      { return this->m_bptr;  }

    const Reference*
    top() const noexcept
      { return this->m_bptr + this->m_etop;  }

    Reference*
    mut_bottom() noexcept
      { return this->m_bptr;  }

    Reference*
    mut_top() noexcept
      { return this->m_bptr + this->m_etop;  }

    Reference_Stack&
    clear() noexcept
      {
        this->m_etop = 0;
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

    const Reference&
    back(size_t index = 0) const noexcept
      {
        ROCKET_ASSERT(index < this->size());
        return this->top()[~index];
      }

    Reference&
    mut_back(size_t index = 0)
      {
        ROCKET_ASSERT(index < this->size());
        return this->mut_top()[~index];
      }

    Reference&
    emplace_back_uninit()
      {
        if(ROCKET_UNEXPECT(this->m_etop == this->m_einit)) {
          // Construct a new reference.
          if(ROCKET_UNEXPECT(this->m_etop == this->m_estor))
            this->do_reserve_more();

          ::rocket::construct_at(this->m_bptr + this->m_einit);
          this->m_einit++;
        }
        return this->m_bptr[this->m_etop++];
      }

    Reference_Stack&
    pop_back(size_t count = 1) noexcept
      {
        ROCKET_ASSERT(count <= this->size());
        this->m_etop -= static_cast<uint32_t>(count);
        return *this;
      }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback) const;
  };

inline void
swap(Reference_Stack& lhs, Reference_Stack& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
