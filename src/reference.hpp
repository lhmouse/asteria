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
		type_direct_reference  = 0,
		type_array_element     = 1,
		type_object_member     = 2,
	};

	using Direct_reference = Value_ptr<Variable>;
	using Array_element    = std::pair<Value_ptr<Variable>, std::int64_t>;
	using Object_member    = std::pair<Value_ptr<Variable>, std::string>;

	using Types = Type_tuple< Direct_reference  // 0
	                        , Array_element     // 1
	                        , Object_member     // 2
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
	Value_ptr<Variable> load() const;
	Value_ptr<Variable> &store(Value_ptr<Variable> &&new_value);
};

}

#endif
