// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RECURSION_SENTRY_
#define ASTERIA_RECURSION_SENTRY_

#include "fwd.hpp"
namespace asteria {

class Recursion_Sentry
  {
  private:
    const void* m_base;

  public:
    constexpr Recursion_Sentry() noexcept
      :
        m_base(this)
      { }

    explicit constexpr Recursion_Sentry(const void* base) noexcept
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
    do_throw_stack_overflow(ptrdiff_t usage, int limit) const;

    void
    do_validate_stack_usage() const
      {
        ptrdiff_t usage = (char*) this->m_base - (char*) this;
        constexpr int mask_bits = 20;  // 1 MiB
        constexpr int ptr_bits = ::std::numeric_limits<ptrdiff_t>::digits;

        if(ROCKET_UNEXPECT(usage >> mask_bits != usage >> ptr_bits))
          this->do_throw_stack_overflow(usage, 1 << mask_bits);
      }

  public:
    // Make this class non-trivial.
    ~Recursion_Sentry()
      {
        __asm__ ("");
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
