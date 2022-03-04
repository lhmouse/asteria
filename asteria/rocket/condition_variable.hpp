// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CONDITION_VARIABLE_HPP_
#define ROCKET_CONDITION_VARIABLE_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include "mutex.hpp"
#include <time.h>

namespace rocket {

class condition_variable;

class condition_variable
  {
  private:
    ::pthread_cond_t m_cond[1] = { PTHREAD_COND_INITIALIZER };

  public:
    constexpr
    condition_variable() noexcept
      { }

    condition_variable(const condition_variable&)
      = delete;

    condition_variable&
    operator=(const condition_variable&)
      = delete;

    ~condition_variable()
      {
        int r = ::pthread_cond_destroy(this->m_cond);
        ROCKET_ASSERT(r == 0);
      }

  private:
    static bool
    do_make_abstime(::timespec& ts, long msecs) noexcept
      {
        // Get the current time.
        int r = ::clock_gettime(CLOCK_REALTIME, &ts);
        ROCKET_ASSERT(r == 0);

        // Ensure we don't cause overflows.
        constexpr int64_t secs_max = noadl::min(INT64_MAX / 1000,
                                         ::std::numeric_limits<::time_t>::max());
        if(msecs > (secs_max - 1 - ts.tv_sec) * 1000)
          return false;

        // If overflow is not possible, add `msecs` to the current time.
        long secs = static_cast<long>(static_cast<unsigned long>(msecs) / 1'000);
        ts.tv_sec += static_cast<::time_t>(secs);

        long mrem = msecs - secs * 1'000;
        if(mrem != 0) {
          ts.tv_nsec += mrem * 1'000'000;

          long mask = static_cast<long>((999'999'999 - ts.tv_nsec) >> 31);
          ts.tv_sec -= mask;
          ts.tv_nsec -= mask & 1'000'000'000;
        }
        return true;
      }

    template<typename mktmT, typename predT>
    bool
    do_wait_check_loop(mutex::unique_lock& lock, mktmT&& make_time, predT&& pred)
      {
        // Release the lock, as the mutex will be unlocked by `do_cond_wait()`
        auto owns = lock.m_sth.release();
        ROCKET_ASSERT(owns);

        // Calculate the time point to give up the wait.
        ::timespec ts;
        if(make_time(ts))
          for(;;) {
            // Wait until `ts`.
            int r = ::pthread_cond_timedwait(this->m_cond, owns, &ts);
            ROCKET_ASSERT(r != EINVAL);

            // Reset the lock, as the mutex shall have been locked again.
            ROCKET_ASSERT(lock.m_sth.get() == nullptr);
            lock.m_sth.reset(owns);

            // Return `true` if the condition has been met.
            if(pred())
              return true;

            // Return `false` if the wait timed out.
            if(r == ETIMEDOUT)
              return false;

            // Restart the wait.
            owns = lock.m_sth.release();
            ROCKET_ASSERT(owns);
          }
        else
          for(;;) {
            // Wait forever.
            int r = ::pthread_cond_wait(this->m_cond, owns);
            ROCKET_ASSERT(r != EINVAL);

            // Reset the lock, as the mutex shall have been locked again.
            ROCKET_ASSERT(lock.m_sth.get() == nullptr);
            lock.m_sth.reset(owns);

            // Return `true` if the condition has been met.
            if(pred())
              return true;

            // Restart the wait.
            owns = lock.m_sth.release();
            ROCKET_ASSERT(owns);
          }
      }

  public:
    void
    wait_for(mutex::unique_lock& lock, long msecs) noexcept
      {
        if(msecs <= 0)
          return;

        this->do_wait_check_loop(lock,
            [&](::timespec& ts) { return this->do_make_abstime(ts, msecs);  },
            [&]() { return true;  });
      }

    template<typename predT>
    bool
    wait_for(mutex::unique_lock& lock, long msecs, predT&& pred) noexcept(noexcept(pred()))
      {
        if(pred())
          return true;

        if(msecs <= 0)
          return false;

        return this->do_wait_check_loop(lock,
            [&](::timespec& ts) { return this->do_make_abstime(ts, msecs);  },
            ::std::forward<predT>(pred));
      }

    void
    wait(mutex::unique_lock& lock) noexcept
      {
        this->do_wait_check_loop(lock,
            [&](::timespec& /*ts*/) { return false;  },
            [&]() { return true;  });
      }

    template<typename predT>
    bool
    wait(mutex::unique_lock& lock, predT&& pred) noexcept(noexcept(pred()))
      {
        if(pred())
          return true;

        return this->do_wait_check_loop(lock,
            [&](::timespec& /*ts*/) { return false;  },
            ::std::forward<predT>(pred));
      }

    void
    notify_one() noexcept
      {
        int r = ::pthread_cond_signal(this->m_cond);
        ROCKET_ASSERT(r == 0);
      }

    void
    notify_all() noexcept
      {
        int r = ::pthread_cond_broadcast(this->m_cond);
        ROCKET_ASSERT(r == 0);
      }
  };

}  // namespace rocket

#endif
