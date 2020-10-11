// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EVALUATION_STACK_HPP_
#define ASTERIA_RUNTIME_EVALUATION_STACK_HPP_

#include "../fwd.hpp"
#include "reference.hpp"

namespace asteria {

class Evaluation_Stack
  {
  private:
    Reference* m_etop;  // this points past the last element
    cow_vector<Reference> m_refs;

  public:
    Evaluation_Stack()
    noexcept
      : m_etop(nullptr), m_refs()
      { }

    ASTERIA_NONCOPYABLE_DESTRUCTOR(Evaluation_Stack);

  public:
    bool
    empty()
    const noexcept
      { return this->m_etop == this->m_refs.data();  }

    size_t
    size()
    const noexcept
      { return static_cast<size_t>(this->m_etop - this->m_refs.data());  }

    Evaluation_Stack&
    clear()
    noexcept
      {
        // We assume that `m_refs` is always empty or owned uniquely, unless it is empty.
        // Reset the top pointer without destroying references for efficiency.
        ROCKET_ASSERT(this->m_refs.empty() || this->m_refs.unique());
        this->m_etop = const_cast<Reference*>(this->m_refs.data());
        return *this;
      }

    Evaluation_Stack&
    reserve(cow_vector<Reference>&& refs)
      {
        // This may throw allocation failure if `refs` is not unique.
        // Reuse the storage of `refs` and initialize the stack to empty.
        this->m_etop = refs.mut_data();
        this->m_refs = ::std::move(refs);
        ROCKET_ASSERT(this->m_etop == this->m_refs.data());
        return *this;
      }

    Evaluation_Stack&
    unreserve(cow_vector<Reference>& refs)
    noexcept
      {
        // This is the inverse of `reserve()`.
        refs = ::std::move(this->m_refs);
        this->m_refs.clear();
        this->m_etop = nullptr;
        return *this;
      }

    const Reference&
    get_top(size_t off = 0)
    const noexcept
      {
        ROCKET_ASSERT(off < this->size());
        return this->m_etop[~off];
      }

    Reference&
    open_top(size_t off = 0)
    noexcept
      {
        ROCKET_ASSERT(off < this->size());
        return this->m_etop[~off];
      }

    template<typename XRefT>
    Reference&
    push(XRefT&& xref)
      {
        // Construct a new reference.
        if(ROCKET_EXPECT(this->size() < this->m_refs.size()))
          this->m_etop[0] = ::std::forward<XRefT>(xref);
        else
          this->m_etop = &(this->m_refs.emplace_back(::std::forward<XRefT>(xref)));

        this->m_etop += 1;
        return this->m_etop[-1];
      }

    Evaluation_Stack&
    pop(size_t cnt = 1)
    noexcept
      {
        // Move the top pointer without destroying elements.
        ROCKET_ASSERT(cnt <= this->size());
        this->m_etop -= cnt;
        return *this;
      }
  };

}  // namespace asteria

#endif
