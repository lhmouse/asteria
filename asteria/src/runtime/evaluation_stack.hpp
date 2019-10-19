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
    Reference* m_etop;
    cow_vector<Reference> m_refs;

  public:
    Evaluation_Stack() noexcept
      :
        m_etop(nullptr), m_refs()
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
    Evaluation_Stack& clear() noexcept
      {
        // We assume that `m_refs` is always owned uniquely, unless it is empty.
        // Reset the top pointer without destroying references for efficiency.
        this->m_etop = this->m_refs.mut_data();
        return *this;
      }
    Evaluation_Stack& reserve(cow_vector<Reference>&& refs)
      {
        // This may throw allocation failure if `refs` is not unique.
        // Reuse the storage of `refs` and initialize the stack to empty.
        this->m_etop = refs.mut_data();
        this->m_refs = rocket::move(refs);
        ROCKET_ASSERT(this->m_etop == this->m_refs.data());
        return *this;
      }

    const Reference& get_top(size_t offset = 0) const noexcept
      {
        ROCKET_ASSERT(offset < this->size());
        return this->m_etop[~offset];
      }
    Reference& open_top(size_t offset = 0) noexcept
      {
        ROCKET_ASSERT(offset < this->size());
        return this->m_etop[~offset];
      }
    template<typename XrefT> Reference& push(XrefT&& xref)
      {
        if(ROCKET_EXPECT(this->size() < this->m_refs.size())) {
          // Overwrite the next element.
          auto& ref = this->m_etop[0];
          ref = rocket::forward<XrefT>(xref);
          this->m_etop++;
          return ref;
        }
        // Push a new element.
        auto& ref = this->m_refs.emplace_back(rocket::forward<XrefT>(xref));
        this->m_etop = std::addressof(ref);
        this->m_etop++;
        return ref;
      }
    Evaluation_Stack& pop(size_t count = 1) noexcept
      {
        ROCKET_ASSERT(count <= this->size());
        this->m_etop -= count;
        return *this;
      }
  };

}  // namespace Asteria

#endif
