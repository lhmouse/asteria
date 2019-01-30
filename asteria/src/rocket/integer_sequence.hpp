// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_INTEGER_SEQUENCE_HPP_
#define ROCKET_INTEGER_SEQUENCE_HPP_

#include <cstddef>  // std::size_t

namespace rocket {

template<typename integerT, integerT ...valuesT> struct integer_sequence
  {
  };

    namespace details_integer_sequence {

    template<typename integerT, unsigned long long countT, integerT ...valuesT> struct make_integer_sequence_helper
      {
        using type = typename make_integer_sequence_helper<integerT, countT - 1, valuesT..., sizeof...(valuesT)>::type;
      };
    template<typename integerT, integerT ...valuesT> struct make_integer_sequence_helper<integerT, 0, valuesT...>
      {
        using type = integer_sequence<integerT, valuesT...>;
      };

    }

template<typename integerT, integerT countT> using make_integer_sequence = typename details_integer_sequence::make_integer_sequence_helper<integerT, countT>::type;
template<size_t countT> using make_index_sequence = make_integer_sequence<size_t, countT>;

template<size_t ...valuesT> using index_sequence = integer_sequence<size_t, valuesT...>;
template<typename ...paramsT> using index_sequence_for = make_index_sequence<sizeof...(paramsT)>;

}

#endif
