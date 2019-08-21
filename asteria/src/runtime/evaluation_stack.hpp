// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_EVALUATION_STACK_HPP_
#define ASTERIA_RUNTIME_EVALUATION_STACK_HPP_

#include "../fwd.hpp"
#include "reference.hpp"

namespace Asteria {

class Evaluation_Stack
  {
  private:
    cow_vector<Reference> m_refs;
    Reference* m_etop;

  public:
    Evaluation_Stack() noexcept
      : m_refs(), m_etop(nullptr)
      {
      }
    ~Evaluation_Stack();

    Evaluation_Stack(const Evaluation_Stack&)
      = delete;
    Evaluation_Stack& operator=(const Evaluation_Stack&)
      = delete;

  public:
    size_t size() const noexcept
      {
        return static_cast<size_t>(this->m_etop - this->m_refs.data());
      }
    void clear() noexcept
      {
        // We assume that `m_refs` is always owned uniquely, unless it is empty.
        auto etop = this->m_refs.mut_data();
        // Reset the top pointer without destroying references for efficiency.
        this->m_etop = etop;
      }
    void reserve(cow_vector<Reference>&& refs)
      {
        // This may throw allocation failure if `refs` is not unique.
        auto etop = refs.mut_data();
        // Reuse the storage of `refs` and initialize the stack to empty.
        this->m_refs = rocket::move(refs);
        this->m_etop = etop;
      }

    const Reference& top() const noexcept
      {
        auto etop = this->m_etop;
        ROCKET_ASSERT(etop);
        ROCKET_ASSERT(etop - this->m_refs.data() >= 1);
        return etop[-1];
      }
    Reference& open_top() noexcept
      {
        auto etop = this->m_etop;
        ROCKET_ASSERT(etop);
        ROCKET_ASSERT(etop - this->m_refs.data() >= 1);
        return etop[-1];
      }
    template<typename XrefT> Reference& push(XrefT&& xref)
      {
        auto etop = this->m_etop;
        if(etop && (etop < this->m_refs.data() + this->m_refs.size())) {
          // Overwrite the next element.
          *etop = rocket::forward<XrefT>(xref);
        }
        else {
          // Push a new element.
          etop = std::addressof(this->m_refs.emplace_back(rocket::forward<XrefT>(xref)));
        }
        // Advance the top pointer past the element that has just been written.
        this->m_etop = ++etop;
        return etop[-1];
      }
    template<typename XvalueT> Reference& set_temporary(bool assign, XvalueT&& xvalue)
      {
        auto etop = this->m_etop;
        ROCKET_ASSERT(etop);
        ROCKET_ASSERT(etop - this->m_refs.data() >= 1);
        if(assign) {
          // Write the value to the top refernce.
          etop[-1].open() = rocket::forward<XvalueT>(xvalue);
        }
        else {
          // Replace the top reference to a temporary reference to the value.
          Reference_Root::S_temporary xref = { rocket::forward<XvalueT>(xvalue) };
          etop[-1] = rocket::move(xref);
        }
        return etop[-1];
      }
    void pop() noexcept
      {
        auto etop = this->m_etop;
        ROCKET_ASSERT(etop);
        ROCKET_ASSERT(etop - this->m_refs.data() >= 1);
        this->m_etop = --etop;
      }
    void pop_next(bool assign)
      {
        auto etop = this->m_etop;
        ROCKET_ASSERT(etop);
        ROCKET_ASSERT(etop - this->m_refs.data() >= 2);
        if(assign) {
          // Read a value from the top reference and write it to the one beneath it.
          etop[-2].open() = etop[-1].read();
        }
        else {
          // Overwrite the reference beneath the top.
          etop[-2] = rocket::move(etop[-1]);
        }
        this->m_etop = --etop;
      }
  };

}  // namespace Asteria

#endif
