// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_STACK_HPP_
#define ASTERIA_RUNTIME_REFERENCE_STACK_HPP_

#include "../fwd.hpp"
#include "reference.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/static_vector.hpp"

namespace Asteria {

class Reference_stack
  {
  private:
    Reference *m_tptr;
    rocket::cow_vector<Reference> m_large;
    rocket::static_vector<Reference, 4> m_small;

  public:
    Reference_stack() noexcept
      : m_large(), m_small()
      {
        this->m_tptr = this->m_small.mut_data();
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Reference_stack);

  public:
    bool empty() const noexcept
      {
        return this->m_tptr == this->m_small.data();
      }
    std::size_t size() const noexcept
      {
        auto tptr = this->m_tptr;
        if((this->m_small.data() <= tptr) && (tptr <= this->m_small.data() + this->m_small.capacity())) {
          // Use the small buffer.
          return static_cast<std::size_t>(tptr - this->m_small.data());
        }
        // Use the large buffer.
        return this->m_small.capacity() + static_cast<std::size_t>(tptr - this->m_large.data());
      }
    void clear() noexcept
      {
        this->m_tptr = this->m_small.mut_data();
      }

    const Reference & top() const noexcept
      {
        auto tptr = this->m_tptr;
        ROCKET_ASSERT(tptr != this->m_small.data());
        ROCKET_ASSERT(tptr != this->m_large.data());
        return tptr[-1];
      }
    Reference & mut_top() noexcept
      {
        auto tptr = this->m_tptr;
        ROCKET_ASSERT(tptr != this->m_small.data());
        ROCKET_ASSERT(tptr != this->m_large.data());
        return tptr[-1];
      }

    template<typename ParamT>
      Reference & push(ParamT &&param)
      {
        auto tptr = this->m_tptr;
        if((this->m_small.data() <= tptr) && (tptr < this->m_small.data() + this->m_small.capacity())) {
          // Use the small buffer.
          if(tptr == this->m_small.data() + this->m_small.size()) {
            tptr = std::addressof(this->m_small.emplace_back(std::forward<ParamT>(param)));
          } else {
            *tptr = std::forward<ParamT>(param);
          }
          ++tptr;
          this->m_tptr = tptr;
          return tptr[-1];
        }
        // Use the large buffer.
        if(tptr == this->m_small.data() + this->m_small.capacity()) {
          tptr = this->m_large.mut_data();
        }
        if(tptr == this->m_large.data() + this->m_large.size()) {
          tptr = std::addressof(this->m_large.emplace_back(std::forward<ParamT>(param)));
        } else {
          *tptr = std::forward<ParamT>(param);
        }
        ++tptr;
        this->m_tptr = tptr;
        return tptr[-1];
      }
    void pop() noexcept
      {
        auto tptr = this->m_tptr;
        ROCKET_ASSERT(tptr != this->m_small.data());
        ROCKET_ASSERT(tptr != this->m_large.data());
        --tptr;
        if(tptr == this->m_large.data()) {
          tptr = this->m_small.mut_data() + this->m_small.capacity();
        }
        this->m_tptr = tptr;
      }
  };

}

#endif
