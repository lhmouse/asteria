// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CONDITION_VARIABLE_HPP_
#define ROCKET_CONDITION_VARIABLE_HPP_

#include "assert.hpp"
#include "utilities.hpp"
#include "mutex.hpp"
#include <time.h>

namespace rocket {

class condition_variable;

#include "details/condition_variable.ipp"

class condition_variable
  {
  private:
    ::pthread_cond_t m_cond = PTHREAD_COND_INITIALIZER;

  public:
    constexpr
    condition_variable()
    noexcept
      { }

    condition_variable(const condition_variable&)
      = delete;

    condition_variable&
    operator=(const condition_variable&)
      = delete;

    ~condition_variable()
      { details_condition_variable::do_cond_destroy(this->m_cond);  }

  public:
    void
    wait(mutex::unique_lock& lock)
    noexcept
      {
        // The mutex will be unlocked, so release the lock.
        auto owns = lock.m_sth.release();
        ROCKET_ASSERT(owns);

        // Now wait on the condition variable.
        details_condition_variable::do_cond_wait(this->m_cond, *owns);

        // The mutex has been locked again, so reset it.
        ROCKET_ASSERT(lock.m_sth.get() == nullptr);
        lock.m_sth.reset(owns);
      }

    template<typename predT>
    void
    wait(mutex::unique_lock& lock, predT&& pred)
    noexcept(noexcept(pred()))
      {
        while(!pred())
          this->wait(lock);
      }

    void
    wait_for(mutex::unique_lock& lock, long msecs)
    noexcept
      {
        // Assume non-positive timeouts expire immediately.
        if(msecs <= 0)
          return;

        // The mutex will be unlocked, so release the lock.
        auto owns = lock.m_sth.release();
        ROCKET_ASSERT(owns);

        // Get the current time.
        ::timespec ts;
        int r = ::clock_gettime(CLOCK_REALTIME, &ts);
        ROCKET_ASSERT(r == 0);

        // Ensure we don't cause overflows.
        int64_t mlim = ::std::numeric_limits<::time_t>::max();
        mlim -= ts.tv_sec + 1;
        mlim *= 1000;

        if(msecs < mlim) {
          // If overflow is not possible, add `msecs` to the current time.
          long secs = static_cast<long>(static_cast<unsigned long>(msecs) / 1'000);
          ts.tv_sec += static_cast<::time_t>(secs);

          long mrem = msecs - secs * 1'000;
          if(mrem != 0) {
            ts.tv_nsec += mrem * 1'000'000;

            long mask = (999'999'999 - ts.tv_nsec) >> 31;
            ts.tv_sec -= mask;
            ts.tv_nsec -= mask & 1'000'000'000;
          }

          // Now wait on the condition variable.
          details_condition_variable::do_cond_timedwait(this->m_cond, *owns, ts);
        }
        else
          // If the value overflows, wait forever.
          details_condition_variable::do_cond_wait(this->m_cond, *owns);

        // The mutex has been locked again, so reset it.
        ROCKET_ASSERT(lock.m_sth.get() == nullptr);
        lock.m_sth.reset(owns);
      }

    template<typename predT>
    void
    wait_for(mutex::unique_lock& lock, long msecs, predT&& pred)
    noexcept(noexcept(pred()))
      {
        while(!pred())
          this->wait_for(lock, msecs);
      }

    void
    notify_one()
    noexcept
      { details_condition_variable::do_cond_signal(this->m_cond);  }

    void
    notify_all()
    noexcept
      { details_condition_variable::do_cond_broadcast(this->m_cond);  }
  };

}  // namespace rocket

#endif
