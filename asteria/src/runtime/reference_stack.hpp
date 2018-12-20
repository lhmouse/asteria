// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_STACK_HPP_
#define ASTERIA_RUNTIME_REFERENCE_STACK_HPP_

#include "../fwd.hpp"
#include "reference.hpp"
#include "../rocket/static_vector.hpp"
#include "../rocket/cow_vector.hpp"

namespace Asteria {

class Reference_stack
  {
  private:
    std::size_t m_size;
    rocket::cow_vector<Reference> m_large;
    rocket::static_vector<Reference, 4> m_small;

  public:
    Reference_stack() noexcept
      : m_size(0), m_large(), m_small()
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Reference_stack);

  public:
    bool empty() const noexcept
      {
        return this->m_size == 0;
      }
    std::size_t size() const noexcept
      {
        return this->m_size;
      }
    void clear() noexcept
      {
        this->m_size = 0;
      }

    const Reference & top() const noexcept
      {
        ROCKET_ASSERT(this->m_size != 0);
        auto off = this->m_size - 1;
        if(ROCKET_EXPECT(off < this->m_small.capacity())) {
          // Use the small buffer.
          ROCKET_ASSERT(off < this->m_small.size());
          return this->m_small[off];
        }
        off -= this->m_small.capacity();
        // Use the large buffer.
        ROCKET_ASSERT(off < this->m_large.size());
        return this->m_large.at(off);
      }
    Reference & top() noexcept
      {
        ROCKET_ASSERT(this->m_size != 0);
        auto off = this->m_size - 1;
        if(ROCKET_EXPECT(off < this->m_small.capacity())) {
          // Use the small buffer.
          ROCKET_ASSERT(off < this->m_small.size());
          return this->m_small[off];
        }
        off -= this->m_small.capacity();
        // Use the large buffer.
        ROCKET_ASSERT(off < this->m_large.size());
        return this->m_large.mut(off);
      }

    template<typename ParamT>
      Reference & push(ParamT &&param)
      {
        auto off = this->m_size;
        if(ROCKET_EXPECT(off < this->m_small.capacity())) {
          // Use the small buffer.
          ROCKET_ASSERT(off <= this->m_small.size());
          auto &ref = (off == this->m_small.size()) ? this->m_small.emplace_back(std::forward<ParamT>(param))
                                                    : this->m_small.at(off) = std::forward<ParamT>(param);
          this->m_size += 1;
          return ref;
        }
        off -= this->m_small.capacity();
        // Use the large buffer.
        ROCKET_ASSERT(off <= this->m_large.size());
        auto &ref = (off == this->m_large.size()) ? this->m_large.emplace_back(std::forward<ParamT>(param))
                                                  : this->m_large.mut(off) = std::forward<ParamT>(param);
        this->m_size += 1;
        return ref;
      }
    void pop() noexcept
      {
        ROCKET_ASSERT(this->m_size != 0);
        this->m_size -= 1;
      }
  };

}

#endif
