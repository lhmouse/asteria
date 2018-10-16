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
    struct Storage
      {
        Storage *prev;
        Storage *next;
        rocket::static_vector<Reference, 16> refs;

        explicit Storage(Storage *xprev) noexcept
          : prev(xprev), next(nullptr), refs()
          {
          }
      };

    Size m_size;
    Storage *m_last;
    Storage m_head;

  public:
    Reference_stack() noexcept
      : m_size(0), m_last(nullptr), m_head(nullptr)
      {
      }
    ~Reference_stack();

    Reference_stack(const Reference_stack &)
      = delete;
    Reference_stack & operator=(const Reference_stack &)
      = delete;

  private:
    Storage * do_reserve_one_more();

  public:
    bool empty() const noexcept
      {
        return this->m_size == 0;
      }
    Size size() const noexcept
      {
        return this->m_size;
      }
    void clear() noexcept
      {
        this->m_size = 0;
        this->m_last = nullptr;
      }

    const Reference & top() const noexcept
      {
        ROCKET_ASSERT(this->m_size != 0);
        auto off = (this->m_size - 1) % this->m_head.refs.capacity();
        auto cur = this->m_last;
        return cur->refs[off];
      }
    Reference & top() noexcept
      {
        ROCKET_ASSERT(this->m_size != 0);
        auto off = (this->m_size - 1) % this->m_head.refs.capacity();
        auto cur = this->m_last;
        return cur->refs[off];
      }
    template<typename ParamT>
      Reference & push(ParamT &&param)
        {
          auto off = this->m_size % this->m_head.refs.capacity();
          auto cur = this->do_reserve_one_more();
          // Create a new reference or reuse an existent one.
          if(off == cur->refs.size()) {
            cur->refs.emplace_back(std::forward<ParamT>(param));
          } else {
            cur->refs[off] = std::forward<ParamT>(param);
          }
          ++(this->m_size);
          this->m_last = cur;
          return cur->refs[off];
        }
    Reference pop() noexcept
      {
        ROCKET_ASSERT(this->m_size != 0);
        --(this->m_size);
        auto off = this->m_size % this->m_head.refs.capacity();
        auto cur = this->m_last;
        auto ref = std::move(cur->refs[off]);
        // Iterate to the previous block if the last block becomes empty.
        if(off == 0) {
          cur = cur->prev;
          this->m_last = cur;
        }
        return ref;
      }
  };

}

#endif
