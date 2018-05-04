// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_EXCHANGE_HPP_
#define ROCKET_EXCHANGE_HPP_

#include <type_traits> // std::decay<>
#include <utility> // std::move(), std::forward()

namespace rocket {

using std::decay;

template<typename elementT>
inline typename decay<elementT>::type exchange(elementT &element_old, const elementT &element_new){
	auto old = ::std::move(element_old);
	element_old = element_new;
	return old;
}
template<typename elementT>
inline typename decay<elementT>::type exchange(elementT &element_old, elementT &&element_new){
	auto old = ::std::move(element_old);
	element_old = ::std::move(element_new);
	return old;
}
template<typename elementT, typename valueT>
inline typename decay<elementT>::type exchange(elementT &element_old, valueT &&value_new){
	auto old = ::std::move(element_old);
	element_old = std::forward<valueT>(value_new);
	return old;
}

}

#endif
