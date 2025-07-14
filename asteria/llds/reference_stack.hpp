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
    uint32_t m_size = 0;
    uint32_t m_estor = 0;

  public:
    constexpr
    Reference_Stack()
      noexcept = default;

    Reference_Stack(Reference_Stack&& other)
      noexcept
      {
        this->swap(other);
      }

    Reference_Stack&
    operator=(Reference_Stack&& other)
      & noexcept
      {
        this->swap(other);
        return *this;
      }

    Reference_Stack&
    swap(Reference_Stack& other)
      noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_size, other.m_size);
        ::std::swap(this->m_estor, other.m_estor);
        return *this;
      }

  private:
    void
    do_reallocate(uint32_t estor);

    void
    do_clear(bool free_storage)
      noexcept;

  public:
    ~Reference_Stack()
      {
        if(this->m_bptr)
          this->do_clear(true);
      }

    uint32_t
    size()
      const noexcept
      {
        return this->m_size;
      }

    void
    clear()
      noexcept
      {
        if(this->m_size != 0)
          this->do_clear(false);
      }

    const Reference&
    top(uint32_t index = 0)
      const noexcept
      {
        ROCKET_ASSERT(index < this->m_size);
        return *(this->m_bptr + this->m_size - 1 - index);
      }

    Reference&
    mut_top(uint32_t index = 0)
      {
        ROCKET_ASSERT(index < this->m_size);
        return *(this->m_bptr + this->m_size - 1 - index);
      }

    const Reference&
    bottom(uint32_t index = 0)
      const noexcept
      {
        ROCKET_ASSERT(index < this->m_size);
        return *(this->m_bptr + index);
      }

    Reference&
    mut_bottom(uint32_t index = 0)
      {
        ROCKET_ASSERT(index < this->m_size);
        return *(this->m_bptr + index);
      }

    Reference&
    push()
      {
#ifdef ROCKET_DEBUG
        this->do_reallocate(this->m_size + 1);
#else
        if(ROCKET_UNEXPECT(this->m_size >= this->m_estor))
          this->do_reallocate(this->m_size * 2 | 17);
#endif

        // Construct a new reference.
        ROCKET_ASSERT(this->m_size < this->m_estor);
        auto ptr = ::rocket::construct(this->m_bptr + this->m_size);
        this->m_size ++;
        return *ptr;
      }

    void
    pop(uint32_t count = 1)
      noexcept
      {
        ROCKET_ASSERT(count <= this->m_size);
        uint32_t target_size = this->m_size - count;
        while(this->m_size != target_size) {
          // Destroy a reference.
          this->m_size --;
          ::rocket::destroy(this->m_bptr + this->m_size);
        }
      }
  };

inline
void
swap(Reference_Stack& lhs, Reference_Stack& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
