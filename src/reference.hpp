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
		type_scoped_variable  = 0,
		type_array_element    = 1,
		type_object_member    = 2,
		type_pure_rvalue      = 3,
	};
	struct Scoped_variable {
		std::shared_ptr<Scope> scope;
		std::string key;
	};
	struct Array_element {
		std::shared_ptr<Variable> variable_opt;
		std::int64_t index_bidirectional;
	};
	struct Object_member {
		std::shared_ptr<Variable> variable_opt;
		std::string key;
	};
	struct Pure_rvalue {
		std::shared_ptr<Variable> variable_opt;
	};
	using Types = Type_tuple< Scoped_variable  // 0
	                        , Array_element    // 1
	                        , Object_member    // 2
	                        , Pure_rvalue      // 3
		>;

private:
	Types::rebound_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Reference, ValueT)>
	Reference(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	~Reference();

public:
	std::shared_ptr<const Variable> load_opt() const;
	Value_ptr<Variable> &store(Value_ptr<Variable> &&new_value_opt);

	Value_ptr<Variable> steal_opt();
};

}

#endif
