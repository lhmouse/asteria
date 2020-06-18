// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_CONDITION_VARIABLE_HPP_
#  error Please include <rocket/condition_variable.hpp> instead.
#endif

namespace details_condition_variable {

inline
void
do_cond_wait(::pthread_cond_t& cond, ::pthread_mutex_t& mutex)
noexcept
  {
    int r = ::pthread_cond_wait(&cond, &mutex);
    ROCKET_ASSERT_MSG(r != EINVAL,
      "Failed to await condition variable (possible corrupted data)");
  }

inline
void
do_cond_timedwait(::pthread_cond_t& cond, ::pthread_mutex_t& mutex, const ::timespec& abstime)
noexcept
  {
    int r = ::pthread_cond_timedwait(&cond, &mutex, &abstime);
    ROCKET_ASSERT_MSG(r != EINVAL,
      "Failed to await condition variable (possible corrupted data)");
  }

inline
void
do_cond_signal(::pthread_cond_t& cond)
noexcept
  {
    int r = ::pthread_cond_signal(&cond);
    ROCKET_ASSERT_MSG(r == 0,
      "Failed to signal condition variable (possible corrupted data)");
  }

inline
void
do_cond_broadcast(::pthread_cond_t& cond)
noexcept
  {
    int r = ::pthread_cond_broadcast(&cond);
    ROCKET_ASSERT_MSG(r == 0,
      "Failed to signal condition variable (possible corrupted data)");
  }

inline
void
do_cond_destroy(::pthread_cond_t& cond)
noexcept
  {
    int r = ::pthread_cond_destroy(&cond);
    ROCKET_ASSERT_MSG(r == 0,
      "Failed to destroy condition variable (possible in use)");
  }

}  // namespace details_condition_variable
