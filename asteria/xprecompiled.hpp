// This file is part of Asteria.
// Copyright (C) 2024-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_XPRECOMPILED_
#define ASTERIA_XPRECOMPILED_

#include "version.h"

#ifdef __FAST_MATH__
#  error Please turn off `-ffast-math`.
#endif

// Prevent use of standard streams.
#define _IOS_BASE_H  1
#define _STREAM_ITERATOR_H  1
#define _STREAMBUF_ITERATOR_H  1
#define _GLIBCXX_ISTREAM  1
#define _GLIBCXX_OSTREAM  1
#define _GLIBCXX_IOSTREAM  1

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
#include "../rocket/xascii.hpp"
#include "../rocket/xhashtable.hpp"
#include "../rocket/xmemory.hpp"
#include "../rocket/xstring.hpp"
#include "../rocket/xuchar.hpp"

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
#include <limits.h>
#include <wchar.h>

#ifdef __SSE2__
#include <x86intrin.h>
#include <emmintrin.h>
#endif

#ifdef __AVX__
#include <immintrin.h>
#endif

#endif
