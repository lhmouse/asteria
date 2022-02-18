// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RECURSION_SENTRY_HPP_
#define ASTERIA_RECURSION_SENTRY_HPP_

#include "fwd.hpp"

namespace asteria {

class Recursion_Sentry
  {
  public:
    enum : size_t {
      stack_mask_bits = 19,  // 512KiB
    };

  private:
    const void* m_base;

  public:
    explicit constexpr
    Recursion_Sentry() noexcept
      : m_base(this)
      { }

    explicit constexpr
    Recursion_Sentry(const void* base) noexcept
      : m_base(base)
      { }

    Recursion_Sentry(const Recursion_Sentry& other)  // copy constructor
      : m_base(other.m_base)
      { this->do_check();  }

    Recursion_Sentry&
    operator=(const Recursion_Sentry& other)  // copy assignment
      { this->m_base = other.m_base;
        this->do_check();
        return *this;  }

  private:
    [[noreturn]] void
    do_throw_stack_overflow(size_t usage, size_t limit) const;

    void
    do_check() const
      {
        // Estimate stack usage.
        size_t usage = static_cast<size_t>(
                         ::std::abs(reinterpret_cast<const char*>(this)
                                    - static_cast<const char*>(this->m_base)));
        if(ROCKET_UNEXPECT(usage >> stack_mask_bits))
          this->do_throw_stack_overflow(usage, size_t(1) << stack_mask_bits);
      }

  public:
    // Make this class non-trivial.
    ~Recursion_Sentry()
      { }

    const void*
    get_base() const noexcept
      { return this->m_base;  }

    Recursion_Sentry&
    set_base(const void* base) noexcept
      { this->m_base = base;  return *this;  }
  };

}  // namespace asteria

#endif
