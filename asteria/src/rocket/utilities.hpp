// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UTILITIES_HPP_
#define ROCKET_UTILITIES_HPP_

#include <type_traits> // std::common_type<>
#include <iterator> // std::begin(), std::end()
#include <utility> // std::swap(), std::move(), std::forward()

namespace rocket {

using ::std::common_type;

template<typename withT, typename typeT>
inline typeT exchange(typeT &ref, withT &&with){
	auto old = ::std::move(ref);
	ref = ::std::forward<withT>(with);
	return old;
}

template<typename lhsT, typename rhsT>
constexpr typename common_type<lhsT &&, rhsT &&>::type min(lhsT &&lhs, rhsT &&rhs){
	return !(rhs < lhs) ? ::std::forward<lhsT>(lhs) : ::std::forward<rhsT>(rhs);
}
template<typename lhsT, typename rhsT>
constexpr typename common_type<lhsT &&, rhsT &&>::type max(lhsT &&lhs, rhsT &&rhs){
	return !(lhs < rhs) ? ::std::forward<lhsT>(lhs) : ::std::forward<rhsT>(rhs);
}

}

#endif
