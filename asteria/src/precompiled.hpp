// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PRECOMPILED_HPP_
#define ASTERIA_PRECOMPILED_HPP_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iterator>
#include <utility>
#include <exception>
#include <typeinfo>
#include <iosfwd>
#include <iomanip>
#include <ostream>
#include <memory>
#include <type_traits>
#include <atomic>
#include <initializer_list>
#include <functional>
#include <algorithm>
#include <array>
#include <tuple>

#include <cstring>
#include <cstddef>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cmath>

#include "rocket/compatibility.h"
#include "rocket/preprocessor_utilities.h"
#include "rocket/assert.hpp"
#include "rocket/throw.hpp"
#include "rocket/utilities.hpp"
#include "rocket/allocator_utilities.hpp"
#include "rocket/insertable_streambuf.hpp"
#include "rocket/insertable_istream.hpp"
#include "rocket/insertable_ostream.hpp"
#include "rocket/insertable_stream.hpp"
#include "rocket/variant.hpp"
#include "rocket/fill_iterator.hpp"
#include "rocket/transparent_comparators.hpp"
#include "rocket/unique_handle.hpp"
#include "rocket/cow_string.hpp"
#include "rocket/cow_vector.hpp"
#include "rocket/cow_hashmap.hpp"
#include "rocket/unique_ptr.hpp"
#include "rocket/refcounted_ptr.hpp"
#include "rocket/refcounted_object.hpp"
#include "rocket/static_vector.hpp"
#include "rocket/prehashed_string.hpp"
#include "rocket/integer_sequence.hpp"
#include "rocket/bind_first.hpp"

#endif
