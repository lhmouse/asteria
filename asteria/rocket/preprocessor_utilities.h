// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_PREPROCESSOR_UTILITIES_H_
#define ROCKET_PREPROCESSOR_UTILITIES_H_

#define ROCKET_CAR(x_, ...)          x_
#define ROCKET_CDR(x_, ...)          __VA_ARGS__
#define ROCKET_QUOTE(...)            #__VA_ARGS__
#define ROCKET_CAT2(x_, y_)          x_##y_
#define ROCKET_CAT3(x_, y_, z_)      x_##y_##z_
#define ROCKET_LAZY(f_, ...)         f_(__VA_ARGS__)

#endif
