// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

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
    rocket::static_vector<Reference, 7> m_small;

  public:
    Reference_stack() noexcept
      : m_large(), m_small()
      {
        // Do not call any member function of `m_small` before its initialization.
        this->m_tptr = this->m_small.mut_data();
      }
    ROCKET_NONCOPYABLE_DESTRUCTOR(Reference_stack);

  private:
    void do_switch_to_large();

  public:
    bool empty() const noexcept
      {
        if(this->m_large.capacity() == 0) {
          // Use `m_small`.
          return m_small.empty();
        }
        // Use `m_large`.
        return m_large.empty();
      }
    std::size_t size() const noexcept
      {
        if(this->m_large.capacity() == 0) {
          // Use `m_small`.
          return static_cast<std::size_t>(this->m_tptr - this->m_small.data());
        }
        // Use `m_large`.
        return static_cast<std::size_t>(this->m_tptr - this->m_large.data());
      }
    void clear() noexcept
      {
        if(this->m_large.capacity() == 0) {
          // Use `m_small`.
          this->m_tptr = m_small.mut_data();
          return;
        }
        // Use `m_large`.
        this->m_tptr = m_large.mut_data();
      }

    const Reference & top() const noexcept
      {
        const auto tptr = this->m_tptr;
        ROCKET_ASSERT(tptr != this->m_small.data());
        ROCKET_ASSERT(tptr != this->m_large.data());
        return tptr[-1];
      }
    Reference & mut_top() noexcept
      {
        const auto tptr = this->m_tptr;
        ROCKET_ASSERT(tptr != this->m_small.data());
        ROCKET_ASSERT(tptr != this->m_large.data());
        return tptr[-1];
      }

    template<typename ParamT>
      Reference & push(ParamT &&param)
      {
        auto tptr = this->m_tptr;
        if(this->m_large.capacity() == 0) {
          // Use `m_small`.
          if(tptr != this->m_small.data() + this->m_small.size()) {
            // Write to the reserved area.
            goto r;
          }
          if(tptr == this->m_small.data() + this->m_small.capacity()) {
            // The small buffer is full.
            this->do_switch_to_large();
            goto x;
          }
          // Extend the small buffer.
          tptr = std::addressof(this->m_small.emplace_back(std::forward<ParamT>(param)));
          ++tptr;
          this->m_tptr = tptr;
          return tptr[-1];
        }
        // Use `m_large`.
        if(tptr != this->m_large.data() + this->m_large.size()) {
          // Write to the reserved area.
      r:
          *tptr = std::forward<ParamT>(param);
          ++tptr;
          this->m_tptr = tptr;
          return tptr[-1];
        }
        // Extend the large buffer.
      x:
        tptr = std::addressof(this->m_large.emplace_back(std::forward<ParamT>(param)));
        ++tptr;
        this->m_tptr = tptr;
        return tptr[-1];
      }
    void pop() noexcept
      {
        auto tptr = this->m_tptr;
        ROCKET_ASSERT(tptr != this->m_small.data());
        ROCKET_ASSERT(tptr != this->m_large.data());
        // Drop an element.
        --tptr;
        this->m_tptr = tptr;
      }
  };

}

#endif
