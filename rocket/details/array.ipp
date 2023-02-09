// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ARRAY_
#  error Please include <rocket/array.hpp> instead.
#endif
namespace details_array {

template<typename valueT, size_t capT, size_t... nestedT>
struct element_type_of
  : identity<array<valueT, nestedT...>>
  { };

template<typename valueT, size_t capT>
struct element_type_of<valueT, capT>
  : identity<valueT>
  { };

}  // namespace details_array
