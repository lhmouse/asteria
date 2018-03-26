// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INITIALIZER_HPP_
#define ASTERIA_INITIALIZER_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Initializer {
public:
	enum Category : unsigned {
		category_bracketed_init_list  = 0,
		category_braced_init_list     = 1,
		category_expression           = 2,
	};

	using Bracketed_init_list = Value_ptr_deque<Initializer>;
	using Braced_init_list    = Value_ptr_map<std::string, Initializer>;
	using Expression_ptr      = Value_ptr<Expression>;

	using Categories = Type_tuple< Bracketed_init_list  // category_bracketed_init_list  = 0,
	                             , Braced_init_list     // category_braced_init_list     = 1,
	                             , Expression_ptr       // category_expression           = 2,
		>;

private:
	Categories::rebound_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Variable, ValueT)>
	Initializer(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	~Initializer();

public:
	Value_ptr<Variable> create_variable() const;
};

}

#endif
