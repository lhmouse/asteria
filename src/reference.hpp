// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_HPP_
#define ASTERIA_REFERENCE_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Reference {
public:
	enum Type : unsigned {
		type_rvalue_generic        = 0,
		type_lvalue_generic        = 1,
		type_lvalue_array_element  = 2,
		type_lvalue_object_member  = 3,
	};
	struct Rvalue_generic {
		std::shared_ptr<Variable> value_opt;
	};
	struct Lvalue_generic {
		std::shared_ptr<Value_ptr<Variable>> variable_ptr;
	};
	struct Lvalue_array_element {
		std::shared_ptr<Variable> variable;
		std::int64_t index_bidirectional;
	};
	struct Lvalue_object_member {
		std::shared_ptr<Variable> variable;
		std::string key;
	};
	using Types = Type_tuple< Rvalue_generic        // 0
	                        , Lvalue_generic        // 1
	                        , Lvalue_array_element  // 2
	                        , Lvalue_object_member  // 3
		>;

private:
	Types::rebound_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Reference, ValueT)>
	Reference(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	~Reference();

	Reference(Reference &&) = default;
	Reference &operator=(Reference &&) = default;

	Reference(const Reference &) = delete;
	Reference &operator=(const Reference &) = delete;

public:
	std::shared_ptr<Variable> load_opt() const;
	void store_opt(boost::optional<Variable> &&value_new_opt) const;
	Value_ptr<Variable> extract_result_opt();
};

}

#endif
