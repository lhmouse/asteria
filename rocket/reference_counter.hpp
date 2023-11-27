// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFERENCE_COUNTER_
#define ROCKET_REFERENCE_COUNTER_

#include "fwd.hpp"
#include "xassert.hpp"
#include <atomic>  // std::atomic<>
#include <exception>  // std::terminate()
namespace rocket {

template<typename valueT = int>
class reference_counter
  {
    static_assert(is_integral<valueT>::value && !is_same<valueT, bool>::value,
        "invalid reference counter value type");

  public:
    using value_type  = valueT;

  private:
ROCKET_PURE static
void*
xtid() noexcept
  {
    void* tid;
    __asm__ ("mov %0, qword ptr fs:[0]" : "=r"(tid));
    return tid;
  }

    ::std::atomic<value_type> m_nref;
    value_type m_local_nref;
    void* m_tid;

  public:
    //constexpr
    reference_counter() noexcept
      :
        m_nref(1), m_local_nref(1), m_tid(xtid())
      { }

    explicit // constexpr
    reference_counter(value_type nref) noexcept
      :
        m_nref(1), m_local_nref(nref), m_tid(xtid())
      { }

    //constexpr
    reference_counter(const reference_counter&) noexcept
      :
        reference_counter()
      { }

    reference_counter&
    operator=(const reference_counter&) & noexcept
      { return *this;  }

    ~reference_counter()
      {
        auto old = this->m_nref.load(memory_order_relaxed);
        if(old > 1)
          ::std::terminate();
      }

  public:
    bool
    unique() const noexcept
      {
        return this->m_nref.load(memory_order_relaxed) == 1
&& this->m_local_nref == 1;
      }

    value_type
    get() const noexcept
      {
if(this->m_local_nref != 0)
  return this->m_local_nref + this->m_nref.load(memory_order_relaxed) - 1;

        return this->m_nref.load(memory_order_relaxed);
      }

    // Increment the counter only if it is non-zero, and return its new value.
    value_type
    try_increment() noexcept
      {
if(this->m_local_nref != 0) {
  this->m_local_nref ++;
  ROCKET_ASSERT(this->m_local_nref >= 2);
  return this->m_local_nref;
}

        auto old = this->m_nref.load(memory_order_relaxed);
        for(;;)
          if(old == 0)
            return old;
          else if(this->m_nref.compare_exchange_weak(old, old + 1,
                                 memory_order_relaxed))
            return old + 1;
      }

    // Increment the counter and return its new value.
    value_type
    increment() noexcept
      {
if(this->m_local_nref != 0) {
  this->m_local_nref ++;
  ROCKET_ASSERT(this->m_local_nref >= 2);
  return this->m_local_nref;
}

        auto old = this->m_nref.fetch_add(1, memory_order_relaxed);
        ROCKET_ASSERT(old >= 1);
        return old + 1;
      }

    // Decrement the counter and return its new value.
    // This has to be done with acquire-release semantics, because the
    // operation which has decremented the counter to zero shall synchronize
    // with other write operations to associated shared resources.
    value_type
    decrement() noexcept
      {
if(this->m_local_nref != 0) {
  this->m_local_nref --;
  if(this->m_local_nref != 0)
    return this->m_local_nref;
}

        auto old = this->m_nref.fetch_sub(1, memory_order_acq_rel);
        ROCKET_ASSERT(old >= 1);
        return old - 1;
      }

    // Provide some convenient operators.
    operator
    value_type() const noexcept
      { return this->get();  }

    value_type
    operator++() noexcept
      { return this->increment();  }

    value_type
    operator--() noexcept
      { return this->decrement();  }

    value_type
    operator++(int) noexcept
      { return this->increment() - 1;  }

    value_type
    operator--(int) noexcept
      { return this->decrement() + 1;  }
  };

}  // namespace rocket
#endif
