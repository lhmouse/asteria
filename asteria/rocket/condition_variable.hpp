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
      {
        while(!pred())
          this->wait(lock);
      }

    void
    wait_for(mutex::unique_lock& lock, double secs)
      {
        // Get the current time.
        ::timespec ts;
        int r = ::clock_gettime(CLOCK_REALTIME, &ts);
        ROCKET_ASSERT(r == 0);

        // Get the end time.
        double end = noadl::clamp(double(ts.tv_sec) + double(ts.tv_nsec) * 1e-9 + secs,
                                  double(0),
                                  double(::std::numeric_limits<::time_t>::max()));

        // Break the time down into second and subsecond parts.
        ts.tv_sec = static_cast<::time_t>(end);
        ts.tv_nsec = static_cast<long>(end - static_cast<double>(ts.tv_sec));

        // The mutex will be unlocked, so release the lock.
        auto owns = lock.m_sth.release();
        ROCKET_ASSERT(owns);

        // Now wait on the condition variable.
        details_condition_variable::do_cond_timedwait(this->m_cond, *owns, ts);

        // The mutex has been locked again, so reset it.
        ROCKET_ASSERT(lock.m_sth.get() == nullptr);
        lock.m_sth.reset(owns);
      }

    template<typename predT>
    void
    wait_for(mutex::unique_lock& lock, double secs, predT&& pred)
      {
        while(!pred())
          this->wait_for(lock, secs);
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
