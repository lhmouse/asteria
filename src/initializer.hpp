// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INITIALIZER_HPP_
#define ASTERIA_INITIALIZER_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"
#include <type_traits> // std::enable_if, std::decay, std::is_base_of

namespace Asteria {

class Initializer {
public:
	enum Category : unsigned {
		category_bracketed_init_list  = 0,
		category_braced_init_list     = 1,
		category_expression           = 2,
	};

	using Types = Type_tuple< Expression_list        // category_bracketed_init_list  = 0
	                        , Key_value_list         // category_braced_init_list     = 1
	                        , Value_ptr<Expression>  // category_expression           = 2
		>;

private:
	Types::rebound_variant m_variant;

public:
	template<typename ValueT, typename std::enable_if<std::is_base_of<Variable, typename std::decay<ValueT>::type>::value == false>::type * = nullptr>
	Initializer(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	~Initializer();

public:
	Category get_category() const noexcept {
		return static_cast<Category>(m_variant.which());
	}

	Value_ptr<Variable> evaluate() const;
};

}

#endif
