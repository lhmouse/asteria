// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_TYPE_TUPLE_HPP_
#define ASTERIA_TYPE_TUPLE_HPP_

#include <boost/variant.hpp>
#include <tuple>

namespace Asteria {

namespace Details {
	template<unsigned indexT, typename ...TypesT>
	struct Type_getter {
		static_assert(indexT && false, "type index out of range");
	};
	template<typename FirstT, typename ...RemainingT>
	struct Type_getter<0, FirstT, RemainingT...> {
		using type = FirstT;
	};
	template<unsigned indexT, typename FirstT, typename ...RemainingT>
	struct Type_getter<indexT, FirstT, RemainingT...> {
		using type = typename Type_getter<indexT - 1, RemainingT...>::type;
	};

	template<unsigned indexT, typename ExpectT, typename ...TypesT>
	struct Type_finder {
		static_assert(indexT && false, "type not found in tuple");
	};
	template<unsigned indexT, typename ExpectT, typename FirstT, typename ...RemainingT>
	struct Type_finder<indexT, ExpectT, FirstT, RemainingT...> {
		enum : unsigned { value = Type_finder<indexT + 1, ExpectT, RemainingT...>::value };
	};
	template<unsigned indexT, typename ExpectT, typename ...RemainingT>
	struct Type_finder<indexT, ExpectT, ExpectT, RemainingT...> {
		enum : unsigned { value = indexT };
	};
}

template<typename ...TypesT>
struct Type_tuple {
	// rebind types to a variant or tuple.
	using rebind_as_variant = boost::variant<TypesT...>;
	using rebind_as_tuple   = std::tuple<TypesT...>;

	// get type by index.
	template<unsigned indexT>
	struct at {
		using type = typename Details::Type_getter<indexT, TypesT...>::type;
	};
	// get index by type.
	template<typename ExpectT>
	struct index_of {
		enum : unsigned { value = Details::Type_finder<0, ExpectT, TypesT...>::value };
	};

	// append and prepend.
	template<typename ...AddT>
	struct prepend {
		using type = Type_tuple<AddT..., TypesT...>;
	};
	template<typename ...AddT>
	struct append {
		using type = Type_tuple<TypesT..., AddT...>;
	};
};

}

#endif
