// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_STACK_HPP_
#define ASTERIA_REFERENCE_STACK_HPP_

#include "fwd.hpp"
#include "reference.hpp"
#include "rocket/static_vector.hpp"

namespace Asteria {

class Reference_stack
  {
  private:
    struct Chunk
      {
        Chunk *prev;
        Chunk *next;
        rocket::static_vector<Reference, 8> refs;

        explicit Chunk(Chunk *xprev) noexcept
          : prev(xprev), next(nullptr), refs()
          {
          }
      };

    Chunk *m_scur;
    std::size_t m_size;
    Chunk m_head;

  public:
    Reference_stack() noexcept
      : m_scur(nullptr), m_size(0), m_head(nullptr)
      {
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Reference_stack);

  private:
    Chunk * do_reserve_one_more();

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
        this->m_scur = nullptr;
        this->m_size = 0;
      }

    const Reference & top() const noexcept
      {
        ROCKET_ASSERT(this->m_size != 0);
        const auto cur = this->m_scur;
        const auto off = (this->m_size - 1) % this->m_head.refs.capacity();
        return cur->refs[off];
      }
    Reference & top() noexcept
      {
        ROCKET_ASSERT(this->m_size != 0);
        const auto cur = this->m_scur;
        const auto off = (this->m_size - 1) % this->m_head.refs.capacity();
        return cur->refs[off];
      }

    template<typename ParamT>
      Reference & push(ParamT &&param)
      {
        auto cur = this->m_scur;
        const auto off = this->m_size % this->m_head.refs.capacity();
        if(off == 0) {
          cur = this->do_reserve_one_more();
        }
        // Create a new reference or reuse an existent one.
        if(off == cur->refs.size()) {
          cur->refs.emplace_back(std::forward<ParamT>(param));
        } else {
          cur->refs[off] = std::forward<ParamT>(param);
        }
        this->m_scur = cur;
        this->m_size += 1;
        ROCKET_ASSERT(this->m_size != 0);
        return cur->refs[off];
      }
    void pop() noexcept
      {
        ROCKET_ASSERT(this->m_size != 0);
        auto cur = this->m_scur;
        const auto off = (this->m_size - 1) % this->m_head.refs.capacity();
        // Do not destroy the element so it can be reused later.
        // Iterate to the previous block if the last block becomes empty.
        if(off == 0) {
          auto prev = cur->prev;
          cur = prev;
        }
        this->m_scur = cur;
        this->m_size -= 1;
      }
  };

}

#endif
