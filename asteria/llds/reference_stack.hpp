// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_STACK_
#define ASTERIA_LLDS_REFERENCE_STACK_

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
      {
        this->swap(other);
      }

    Reference_Stack&
    operator=(Reference_Stack&& other) & noexcept
      {
        return this->swap(other);
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
    // This is the only memory management function. `estor` shall specify the
    // new number of references to reserve. If `estor` is zero, any dynamic
    // storage will be deallocated.
    void
    do_reallocate(uint32_t estor);

  public:
    ~Reference_Stack()
      {
        if(this->m_bptr)
          this->do_reallocate(0);
      }

    bool
    empty() const noexcept
      { return this->m_etop == 0;  }

    size_t
    size() const noexcept
      { return this->m_etop;  }

    void
    clear() noexcept
      {
        this->m_etop = 0;
      }

    void
    clear_cache() noexcept;

    const Reference&
    top(size_t index = 0) const noexcept
      {
        ROCKET_ASSERT(index < this->m_etop);
        return *(this->m_bptr + this->m_etop - 1 - index);
      }

    Reference&
    mut_top(size_t index = 0)
      {
        ROCKET_ASSERT(index < this->m_etop);
        return *(this->m_bptr + this->m_etop - 1 - index);
      }

    Reference&
    push()
      {
        if(ROCKET_UNEXPECT(this->m_etop >= this->m_einit)) {
          // Construct a new reference.
          if(this->m_einit >= this->m_estor)
            this->do_reallocate(this->m_estor / 2 * 3 | 17);

          ::rocket::construct(this->m_bptr + this->m_einit);
          this->m_einit ++;
          this->m_etop = this->m_einit;
        }
        else {
          // Reuse a previous one.
          this->m_etop ++;
        }
        return *(this->m_bptr + this->m_etop - 1);
      }

    void
    pop(size_t count = 1) noexcept
      {
        ROCKET_ASSERT(count <= this->m_etop);
        this->m_etop -= (uint32_t) count;
      }

    void
    get_variables(Variable_HashMap& staged, Variable_HashMap& temp) const;
  };

inline
void
swap(Reference_Stack& lhs, Reference_Stack& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace asteria
#endif
