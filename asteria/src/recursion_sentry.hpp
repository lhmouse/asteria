// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RECURSION_SENTRY_HPP_
#define ASTERIA_RECURSION_SENTRY_HPP_

#include "fwd.hpp"

namespace Asteria {

class Recursion_Sentry
  {
  public:
    enum : size_t
      {
        max_stack_usage  = 0x3FFFF  // 256 KiB
      };

  private:
    const void* m_base;

  public:
    constexpr Recursion_Sentry() noexcept
      :
        m_base(this)
      {
      }
    constexpr Recursion_Sentry(const void* base) noexcept
      :
        m_base(base)
      {
      }
    Recursion_Sentry(const Recursion_Sentry& other)  // copy constructor
      :
        m_base(other.m_base)
      {
        // Estimate stack usage.
        size_t usage = static_cast<size_t>(::std::abs(reinterpret_cast<const char*>(this) -
                                                       reinterpret_cast<const char*>(this->m_base)));
        if(ROCKET_UNEXPECT(usage >= max_stack_usage))
          this->do_throw_stack_overflow(usage, max_stack_usage);
      }
    ~Recursion_Sentry()
      {
      }
    Recursion_Sentry& operator=(const Recursion_Sentry&)  // not assignable
      = delete;

  private:
    [[noreturn]] void do_throw_stack_overflow(size_t usage, size_t limit);

  public:
    const void* get_base() const noexcept
      {
        return this->m_base;
      }
    void set_base(const void* base) noexcept
      {
        this->m_base = base;
      }
  };

}  // namespace Asteria

#endif
