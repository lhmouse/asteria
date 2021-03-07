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
    Reference_Stack()
      noexcept
      { }

    Reference_Stack(Reference_Stack&& other)
      noexcept
      { this->swap(other);  }

    Reference_Stack&
    operator=(Reference_Stack&& other)
      noexcept
      { return this->swap(other);  }

  private:
    void
    do_destroy_elements()
      noexcept;

    void
    do_reallocate_reserve(Reference*& bptr, uint32_t& estor, uint32_t nadd)
      const;

    void
    do_reallocate_finish(Reference* bptr, uint32_t estor)
      noexcept;

    template<typename XRefT>
    ROCKET_NOINLINE
    Reference&
    do_reallocate_emplace_back(XRefT&& xref)
      {
        // Allocate a larger block of memory and then construct the new element
        // above the top. If the operation succeeds, replace the old block.
        uint32_t nadd = 1;
#ifndef ROCKET_DEBUG
        // Reserve more space for non-debug builds.
        nadd |= 31;
        nadd |= this->m_estor * 2;
#endif
        Reference* bptr;
        uint32_t estor;
        this->do_reallocate_reserve(bptr, estor, nadd);

        auto ptr = bptr + this->m_etop;
        try {
          ::rocket::construct_at(ptr, ::std::forward<XRefT>(xref));
        }
        catch(...) {
          ::operator delete(bptr);
          throw;
        }
        this->do_reallocate_finish(bptr, estor);
        this->m_etop += 1;
        this->m_einit += 1;
        return *ptr;
      }

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
    empty()
      const noexcept
      { return this->m_etop == 0;  }

    size_t
    size()
      const noexcept
      { return this->m_etop;  }

    const Reference*
    bottom()
      const noexcept
      { return this->m_bptr;  }

    Reference*
    mut_bottom()
      noexcept
      { return this->m_bptr;  }

    Reference_Stack&
    clear()
      noexcept
      {
        this->m_etop = 0;
        return *this;
      }

    Reference_Stack&
    swap(Reference_Stack& other)
      noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_etop, other.m_etop);
        ::std::swap(this->m_einit, other.m_einit);
        ::std::swap(this->m_estor, other.m_estor);
        return *this;
      }

    const Reference&
    back(size_t index = 0)
      const noexcept
      {
        ROCKET_ASSERT(index < this->size());
        return this->bottom()[this->size() + ~index];
      }

    Reference&
    mut_back(size_t index = 0)
      {
        ROCKET_ASSERT(index < this->size());
        return this->mut_bottom()[this->size() + ~index];
      }

    template<typename XRefT>
    ROCKET_FORCED_INLINE_FUNCTION
    Reference&
    emplace_back(XRefT&& xref)
      {
        // If there is an initialized element above the top, reuse it.
        if(ROCKET_EXPECT(this->m_etop != this->m_einit)) {
          auto ptr = this->m_bptr + this->m_etop;
          *ptr = ::std::forward<XRefT>(xref);
          this->m_etop += 1;
          return *ptr;
        }

        // If there is an uninitialized element above the top, construct a new one.
        if(ROCKET_EXPECT(this->m_etop != this->m_estor)) {
          auto ptr = this->m_bptr + this->m_etop;
          ::rocket::construct_at(ptr, ::std::forward<XRefT>(xref));
          this->m_etop += 1;
          this->m_einit += 1;
          return *ptr;
        }

        return this->do_reallocate_emplace_back(::std::forward<XRefT>(xref));
      }

    Reference_Stack&
    pop_back(size_t count = 1)
      noexcept
      {
        ROCKET_ASSERT(count <= this->size());
        this->m_etop -= static_cast<uint32_t>(count);
        return *this;
      }

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
      const;
  };

inline
void
swap(Reference_Stack& lhs, Reference_Stack& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
