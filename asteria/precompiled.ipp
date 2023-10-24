// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PRECOMPILED_
#define ASTERIA_PRECOMPILED_

#include "version.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef DLL_EXPORT
#  define ASTERIA_DLLEXPORT  __declspec(dllexport)
#else
#  define ASTERIA_DLLEXPORT
#endif

#ifdef __FAST_MATH__
#  error Please turn off `-ffast-math`.
#endif

#include "../rocket/cow_string.hpp"
#include "../rocket/tinyfmt_str.hpp"
#include "../rocket/cow_vector.hpp"
#include "../rocket/cow_hashmap.hpp"
#include "../rocket/static_vector.hpp"
#include "../rocket/prehashed_string.hpp"
#include "../rocket/unique_handle.hpp"
#include "../rocket/unique_posix_file.hpp"
#include "../rocket/unique_posix_dir.hpp"
#include "../rocket/unique_posix_fd.hpp"
#include "../rocket/unique_ptr.hpp"
#include "../rocket/refcnt_ptr.hpp"
#include "../rocket/variant.hpp"
#include "../rocket/optional.hpp"
#include "../rocket/array.hpp"
#include "../rocket/reference_wrapper.hpp"
#include "../rocket/tinyfmt.hpp"
#include "../rocket/tinyfmt_str.hpp"
#include "../rocket/tinyfmt_file.hpp"
#include "../rocket/ascii_numget.hpp"
#include "../rocket/ascii_numput.hpp"
#include "../rocket/atomic.hpp"

#include <iterator>
#include <utility>
#include <exception>
#include <typeinfo>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <array>

#include <climits>
#include <cmath>
#include <cfenv>
#include <cfloat>
#include <cstring>
#include <cerrno>

#include <sys/types.h>
#include <unistd.h>
#include <x86intrin.h>
#include <immintrin.h>

#endif
