// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_PREPROCESSOR_UTILITIES_H_
#define ROCKET_PREPROCESSOR_UTILITIES_H_

#define ROCKET_CAR(x, ...)          x
#define ROCKET_CDR(x, ...)          __VA_ARGS__
#define ROCKET_QUOTE(...)           #__VA_ARGS__
#define ROCKET_CAT2(x, y)           x##y
#define ROCKET_CAT3(x, y, z)        x##y##z
#define ROCKET_LAZY(f, ...)         f(__VA_ARGS__)

#endif
