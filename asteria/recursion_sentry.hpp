// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RECURSION_SENTRY_
#define ASTERIA_RECURSION_SENTRY_

#include "fwd.hpp"
namespace asteria {

class Recursion_Sentry
  {
  public:
    enum : uint32_t
      {
        stack_mask_bits = 19,  // 512KiB
      };

  private:
    const void* m_base;

  public:
    constexpr
    Recursion_Sentry() noexcept
      :
        m_base(this)
      { }

    constexpr
    explicit Recursion_Sentry(const void* base) noexcept
      :
        m_base(base)
      { }

    Recursion_Sentry(const Recursion_Sentry& other)  // copy constructor
      :
        m_base(other.m_base)
      { this->do_validate_stack_usage();  }

    Recursion_Sentry&
    operator=(const Recursion_Sentry& other) &  // copy assignment
      {
        this->m_base = other.m_base;
        this->do_validate_stack_usage();
        return *this;
      }

  private:
    [[noreturn]]
    void
    do_throw_stack_overflow(ptrdiff_t usage) const;

    void
    do_validate_stack_usage() const
      {
        ptrdiff_t usage = (char*) this->m_base - (char*) this;
        if(ROCKET_UNEXPECT(((usage >> stack_mask_bits) + 1) >> 1 != 0))
          this->do_throw_stack_overflow(usage);
      }

  public:
    // Make this class non-trivial.
    ~Recursion_Sentry()
      {
        __asm__ volatile ("" :);
      }

    const void*
    get_base() const noexcept
      {
        return this->m_base;
      }

    Recursion_Sentry&
    set_base(const void* base) noexcept
      {
        this->m_base = base;
        return *this;
      }
  };

}  // namespace asteria
#endif
