// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_PREPROCESSOR_UTILITIES_H_
#define ROCKET_PREPROCESSOR_UTILITIES_H_

#define ROCKET_CAR(x_, ...)          x_
#define ROCKET_CDR(x_, ...)          __VA_ARGS__
#define ROCKET_QUOTE(...)            #__VA_ARGS__
#define ROCKET_CAT2(x_, y_)          x_##y_
#define ROCKET_CAT3(x_, y_, z_)      x_##y_##z_
#define ROCKET_LAZY(f_, ...)         f_(__VA_ARGS__)

#define ROCKET_DECLARE_COPYABLE_DESTRUCTOR(c_, ...)  \
  c_(const c_ &)              = default;  \
  c_ & operator=(const c_ &)  = default;  \
  c_(c_ &&)                   = default;  \
  c_ & operator=(c_ &&)       = default;  \
  __VA_ARGS__ ~c_()

#define ROCKET_DECLARE_MOVABLE_DESTRUCTOR(c_, ...)  \
  c_(const c_ &)              = delete;  \
  c_ & operator=(const c_ &)  = delete;  \
  c_(c_ &&)                   = default;  \
  c_ & operator=(c_ &&)       = default;  \
  __VA_ARGS__ ~c_()

#define ROCKET_DECLARE_NONCOPYABLE_DESTRUCTOR(c_, ...)  \
  c_(const c_ &)              = delete;  \
  c_ & operator=(const c_ &)  = delete;  \
  c_(c_ &&)                   = delete;  \
  c_ & operator=(c_ &&)       = delete;  \
  __VA_ARGS__ ~c_()

#endif
