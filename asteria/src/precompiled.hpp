// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PRECOMPILED_HPP_
#define ASTERIA_PRECOMPILED_HPP_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "../rocket/preprocessor_utilities.h"
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
#include "../rocket/refcnt_object.hpp"
#include "../rocket/variant.hpp"
#include "../rocket/optional.hpp"
#include "../rocket/array.hpp"
#include "../rocket/reference_wrapper.hpp"
#include "../rocket/tinybuf.hpp"
#include "../rocket/tinybuf_str.hpp"
#include "../rocket/tinybuf_file.hpp"
#include "../rocket/tinyfmt.hpp"
#include "../rocket/tinyfmt_str.hpp"
#include "../rocket/tinyfmt_file.hpp"
#include "../rocket/atomic_flag.hpp"

#include <iterator>
#include <utility>
#include <exception>
#include <typeinfo>
#include <type_traits>
#include <functional>
#include <algorithm>

#include <climits>
#include <cmath>
#include <cfenv>
#include <cfloat>
#include <cstring>

#endif
