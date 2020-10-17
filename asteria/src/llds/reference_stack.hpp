// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_REFERENCE_STACK_HPP_
#define ASTERIA_LLDS_REFERENCE_STACK_HPP_

#include "../fwd.hpp"
#include "../runtime/reference.hpp"

namespace asteria {

class Reference_Stack
  {
  private:
    Reference* m_bptr = nullptr;  // beginning of raw storage
    uint32_t m_bstor = 0;  // offset to the first initialized reference
    uint32_t m_top = 0;  // offset to the top (must be within [m_bstor,m_estor])
    uint32_t m_estor = 0;  // number of references allocated

  public:
    constexpr
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

    ~Reference_Stack()
      {
        if(this->m_bstor != this->m_estor)
          this->do_destroy_elements();

        if(this->m_bptr)
          ::operator delete(this->m_bptr);

#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this), 0xBA, sizeof(*this));
#endif
      }

  private:
    [[noreturn]] inline
    void
    do_throw_subscript_out_of_range(size_t index, const char* rel)
      const
      {
        ::rocket::sprintf_and_throw<::std::out_of_range>(
                       "Reference_Stack: Subscript out of range (`%zu` %s `%zu`)",
                       index, rel, this->size());
      }

    void
    do_destroy_elements()
      noexcept;

    void
    do_reallocate_reserve(Reference*& bptr, uint32_t& estor, uint32_t nadd)
      const;

    void
    do_reallocate_finish(Reference* bptr, uint32_t estor)
      noexcept;

  public:
    bool
    empty()
      const noexcept
      { return this->m_top == this->m_estor;  }

    size_t
    size()
      const noexcept
      { return this->m_estor - this->m_top;  }

    const Reference*
    data()
      const noexcept
      { return this->m_bptr + this->m_top;  }

    Reference*
    mut_data()
      noexcept
      { return this->m_bptr + this->m_top;  }

    const Reference*
    begin()
      const noexcept
      { return this->m_bptr + this->m_top;  }

    const Reference*
    end()
      const noexcept
      { return this->m_bptr + this->m_estor;  }

    Reference*
    begin()
      noexcept
      { return this->m_bptr + this->m_top;  }

    Reference*
    end()
      noexcept
      { return this->m_bptr + this->m_estor;  }

    Reference_Stack&
    clear()
      noexcept
      {
        this->m_top = this->m_estor;
        return *this;
      }

    Reference_Stack&
    swap(Reference_Stack& other)
      noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_bstor, other.m_bstor);
        ::std::swap(this->m_top, other.m_top);
        ::std::swap(this->m_estor, other.m_estor);
        return *this;
      }

    const Reference&
    at(size_t index)
      const
      {
        if(index >= this->size())
          this->do_throw_subscript_out_of_range(index, ">=");
        return this->data()[index];
      }

    const Reference&
    rat(size_t index)
      const
      {
        if(index >= this->size())
          this->do_throw_subscript_out_of_range(index, ">=");
        return this->data()[this->size() - 1 - index];
      }

    const Reference&
    front()
      const noexcept
      {
        ROCKET_ASSERT(!this->empty());
        return this->data()[0];
      }

    const Reference&
    back()
      const noexcept
      {
        ROCKET_ASSERT(!this->empty());
        return this->data()[this->size() - 1];
      }

    Reference&
    mut(size_t index)
      {
        if(index >= this->size())
          this->do_throw_subscript_out_of_range(index, ">=");
        return this->mut_data()[index];
      }

    Reference&
    rmut(size_t index)
      {
        if(index >= this->size())
          this->do_throw_subscript_out_of_range(index, ">=");
        return this->mut_data()[this->size() - 1 - index];
      }

    Reference&
    mut_front()
      noexcept
      {
        ROCKET_ASSERT(!this->empty());
        return this->mut_data()[0];
      }

    Reference&
    mut_back()
      noexcept
      {
        ROCKET_ASSERT(!this->empty());
        return this->mut_data()[this->size() - 1];
      }

    template<typename XRefT>
    Reference&
    emplace_front(XRefT&& xref)
      {
        // If there is an initialized element above the top, reuse it.
        if(ROCKET_EXPECT(this->m_bstor != this->m_top)) {
          auto ptr = this->m_bptr + this->m_top - 1;
          *ptr = ::std::forward<XRefT>(xref);
          this->m_top -= 1;
          return *ptr;
        }

        // If there is an uninitialized element above the top, construct a
        // new one.
        if(ROCKET_EXPECT(this->m_bstor != 0)) {
          auto ptr = this->m_bptr + this->m_top - 1;
          ::rocket::construct_at(ptr, ::std::forward<XRefT>(xref));
          this->m_bstor -= 1;
          this->m_top -= 1;
          return *ptr;
        }

        // Allocate a larger block of memory and then construct the new
        // element  above the top. If the operation succeeds, replace the
        // old block.
        Reference* next_bptr;
        uint32_t next_estor;

        uint32_t nadd = 1;
#ifndef ROCKET_DEBUG
        // Reserve more space for non-debug builds.
        nadd |= 31;
        nadd |= this->m_estor * 2;
#endif
        this->do_reallocate_reserve(next_bptr, next_estor, nadd);

        auto ptr = next_bptr + next_estor - this->m_estor + this->m_top - 1;
        try {
          ::rocket::construct_at(ptr, ::std::forward<XRefT>(xref));
        }
        catch(...) {
          ::operator delete(next_bptr);
          throw;
        }
        this->do_reallocate_finish(next_bptr, next_estor);
        this->m_bstor -= 1;
        this->m_top -= 1;
        return *ptr;
      }

    Reference_Stack&
    pop_front(size_t count = 1)
      noexcept
      {
        ROCKET_ASSERT(count <= this->size());
        this->m_top += static_cast<uint32_t>(count);
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
