// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ARRAY_HPP_
#  error Please include <rocket/array.hpp> instead.
#endif

namespace details_array {

template<typename valueT, size_t capacityT, size_t... nestedT>
    struct element_type_of
      : enable_if<1, array<valueT, nestedT...>>
  { };

template<typename valueT, size_t capacityT>
    struct element_type_of<valueT, capacityT>
      : enable_if<1, valueT>
  { };

}  // namespace details_array
