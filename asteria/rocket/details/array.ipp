// This file is part of Asteria.
// Copyright (C) 2018-2026 LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ROCKET_ARRAY_
#  error Please include <asteria/array.hpp> instead.
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
