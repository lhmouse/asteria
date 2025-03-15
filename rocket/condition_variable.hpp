// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CONDITION_VARIABLE_
#define ROCKET_CONDITION_VARIABLE_

#include "fwd.hpp"
#include "mutex.hpp"
#include "xassert.hpp"
#include <condition_variable>
#include <time.h>
namespace rocket {

class condition_variable
  {
  private:
    ::std::condition_variable m_std_cond;

  public:
    condition_variable() = default;

    condition_variable(const condition_variable&) = delete;
    condition_variable& operator=(const condition_variable&) = delete;

  public:
    // These interfaces reassemble the condition variable on Windows.
    void
    wait(mutex::unique_lock& lock)
      {
        ROCKET_ASSERT(lock.m_mtx);

        // Release `lock.m_mtx` before the wait operation, as it might get
        // modified by other threads.
        ::pthread_mutex_t* saved_mtx = lock.m_mtx;
        lock.m_mtx = nullptr;
        ::pthread_cond_wait(this->m_std_cond.native_handle(), saved_mtx);
        lock.unlock();
        lock.m_mtx = saved_mtx;
      }

    template<typename durationT>
    void
    wait_for(mutex::unique_lock& lock, const durationT& timeout)
      {
        ROCKET_ASSERT(lock.m_mtx);

        // Convert the duration to a time point.
        ::timespec ts;
        ::clock_gettime(CLOCK_REALTIME, &ts);
        double secs = (double) ts.tv_sec + (double) ts.tv_nsec * 0.000000001;

        using hires_secs = ::std::chrono::duration<double>;
        secs += ::std::chrono::duration_cast<hires_secs>(timeout).count();
        constexpr int64_t tm_max = 0x7FFFFFFFFFFFFC00;

        if(secs >= tm_max) {
          ts.tv_sec = tm_max;
          ts.tv_nsec = 0;
        }
        else if(secs <= 0) {
          ts.tv_sec = 0;
          ts.tv_nsec = 0;
        }
        else {
          secs += 0.000000000999;
          ts.tv_sec = (::time_t) secs;
          ts.tv_nsec = (long) ((secs - (double) ts.tv_sec) * 1000000000);
        }

        // Release `lock.m_mtx` before the wait operation, as it might get
        // modified by other threads.
        ::pthread_mutex_t* saved_mtx = lock.m_mtx;
        lock.m_mtx = nullptr;
        ::pthread_cond_timedwait(this->m_std_cond.native_handle(), saved_mtx, &ts);
        lock.unlock();
        lock.m_mtx = saved_mtx;
      }

    void
    notify_one() noexcept
      {
        ::pthread_cond_signal(this->m_std_cond.native_handle());
      }

    void
    notify_all() noexcept
      {
        ::pthread_cond_broadcast(this->m_std_cond.native_handle());
      }
  };

}  // namespace rocket
#endif
