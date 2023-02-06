// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CONDITION_VARIABLE_
#define ROCKET_CONDITION_VARIABLE_

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
#ifdef __linux__
    constexpr condition_variable() noexcept = default;
#else
    condition_variable() noexcept = default;
    ~condition_variable() { ::pthread_cond_destroy(this->m_cond);  }
#endif

    condition_variable(const condition_variable&) = delete;
    condition_variable& operator=(const condition_variable&) = delete;

  private:
    ::timespec
    do_abs_timeout(int64_t ms) noexcept
      {
        ::timespec ts;
        double value = (double) ms * 0.001 + 0.0009999999;

        int r = ::clock_gettime(CLOCK_REALTIME, &ts);
        ROCKET_ASSERT(r == 0);
        value += (double) ts.tv_sec + (double) ts.tv_nsec * 0.000000001;

        ts.tv_sec = ::std::numeric_limits<::time_t>::max();
        ts.tv_nsec = 0;

        if(value >= (double) ts.tv_sec)
          return ts;  // overflowed

        ts.tv_sec = (time_t) value;
        ts.tv_nsec = (long) ((value - (double) ts.tv_sec) * 1000000000);
        return ts;
      }

  public:
    bool
    wait(mutex::unique_lock& lock) noexcept
      {
        auto owns = lock.m_sth.release();
        ROCKET_ASSERT(owns);

        int r = ::pthread_cond_wait(this->m_cond, owns);
        ROCKET_ASSERT((r == 0) || (r == EINTR));

        ROCKET_ASSERT(lock.m_sth.get() == nullptr);
        lock.m_sth.reset(owns);
        return r == 0;
      }

    template<typename predT>
    void
    wait(mutex::unique_lock& lock, predT&& pred) noexcept(noexcept(pred()))
      {
        while(!pred())
          this->wait(lock);
      }

    bool
    wait_until(mutex::unique_lock& lock, const ::timespec& ts) noexcept
      {
        auto owns = lock.m_sth.release();
        ROCKET_ASSERT(owns);

        int r = ::pthread_cond_timedwait(this->m_cond, owns, &ts);
        ROCKET_ASSERT((r == 0) || (r == EINTR) || (r == ETIMEDOUT));

        ROCKET_ASSERT(lock.m_sth.get() == nullptr);
        lock.m_sth.reset(owns);
        return r == 0;
      }

    template<typename predT>
    void
    wait_until(mutex::unique_lock& lock, const ::timespec& ts, predT&& pred) noexcept(noexcept(pred()))
      {
        while(!pred())
          this->wait_until(lock, ts);
      }

    bool
    wait_for(mutex::unique_lock& lock, int64_t ms) noexcept
      {
        return this->wait_until(lock, this->do_abs_timeout(ms));
      }

    template<typename predT>
    void
    wait_for(mutex::unique_lock& lock, int64_t ms, predT&& pred) noexcept(noexcept(pred()))
      {
        return this->wait_until(lock, this->do_abs_timeout(ms), ::std::forward<predT>(pred));
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
