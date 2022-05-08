// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FORMAT_
#  error Please include <rocket/format.hpp> instead.
#endif

namespace details_format {

template<typename charT, typename traitsT, typename valueT>
void
default_insert(basic_tinyfmt<charT, traitsT>& fmt, const void* ptr)
  {
    fmt << *(static_cast<const valueT*>(ptr));
  }

}  // namespace details_format

