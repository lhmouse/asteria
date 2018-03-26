// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TYPE_TUPLE_HPP_
#define ASTERIA_TYPE_TUPLE_HPP_

#include <boost/variant.hpp>
#include <tuple>
#include <type_traits>

namespace Asteria {

namespace Details {
	template<unsigned subscriptT, typename ExpectT, typename ...TypesT>
	struct Type_finder;

	template<unsigned subscriptT, typename ExpectT, typename FirstT, typename ...RemainingT>
	struct Type_finder<subscriptT, ExpectT, FirstT, RemainingT...> {
		enum : unsigned { value = Type_finder<subscriptT + 1, ExpectT, RemainingT...>::value };
	};
	template<unsigned subscriptT, typename ExpectT, typename ...RemainingT>
	struct Type_finder<subscriptT, ExpectT, ExpectT, RemainingT...> {
		enum : unsigned { value = subscriptT };
	};
	template<unsigned subscriptT, typename ExpectT>
	struct Type_finder<subscriptT, ExpectT> {
		static_assert(bool{(subscriptT, false)}, "type not found in tuple");
	};
}

template<typename ...TypesT>
struct Type_tuple {
	// rebind types to a variant or tuple.
	using rebound_variant = boost::variant<TypesT...>;
	using rebound_tuple   = std::tuple<TypesT...>;

	// get type by index.
	template<unsigned indexT>
	struct at {
		using type = typename std::tuple_element<indexT, rebound_tuple>::type;
	};
	// get index by type.
	template<typename ExpectT>
	struct index_of {
		enum : unsigned { value = Details::Type_finder<0, ExpectT, TypesT...>::value };
	};
};

}

#endif
