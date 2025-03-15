// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ARRAY_
#  error Please include <rocket/array.hpp> instead.
#endif
namespace details_array {

template<typename valueT, size_t capT, size_t... nestedT>
struct element_type_of
  {
    using type = array<valueT, nestedT...>;
  };

template<typename valueT, size_t capT>
struct element_type_of<valueT, capT>
  {
    using type = valueT;
  };

}  // namespace details_array
