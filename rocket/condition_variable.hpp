// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CONDITION_VARIABLE_
#define ROCKET_CONDITION_VARIABLE_

#include "fwd.hpp"
#include "mutex.hpp"
#include "assert.hpp"
#include <condition_variable>
namespace rocket {

class condition_variable
  {
  private:
    ::std::condition_variable m_cond;

  public:
    condition_variable() = default;

    condition_variable(const condition_variable&) = delete;
    condition_variable& operator=(const condition_variable&) = delete;

  public:
    // These interfaces reassemble the condition variable on Windows.
    void
    wait(mutex::unique_lock& lock)
      {
        ROCKET_ASSERT(lock.m_mtx != nullptr);
        ::std::unique_lock<::std::mutex> sl(*(lock.m_mtx), ::std::adopt_lock);
        lock.m_mtx = nullptr;

        this->m_cond.wait(sl);

        lock.unlock();
        lock.m_mtx = sl.release();
      }

    template<typename durationT>
    void
    wait_for(mutex::unique_lock& lock, const durationT& timeout)
      {
        ROCKET_ASSERT(lock.m_mtx != nullptr);
        ::std::unique_lock<::std::mutex> sl(*(lock.m_mtx), ::std::adopt_lock);
        lock.m_mtx = nullptr;

        this->m_cond.wait_for(sl, timeout);

        lock.unlock();
        lock.m_mtx = sl.release();
      }

    void
    notify_one() noexcept
      {
        this->m_cond.notify_one();
      }

    void
    notify_all() noexcept
      {
        this->m_cond.notify_all();
      }
  };

}  // namespace rocket
#endif
