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
		type_null_reference    = 0,
		type_direct_reference  = 1,
		type_array_element     = 2,
		type_object_member     = 3,
	};

	using Null_reference   = std::nullptr_t;
	using Direct_reference = std::shared_ptr<Variable>;
	using Array_element    = std::pair<std::shared_ptr<Variable>, std::int64_t>;
	using Object_member    = std::pair<std::shared_ptr<Variable>, std::string>;

	using Types = Type_tuple< Null_reference    // 0
	                        , Direct_reference  // 1
	                        , Array_element     // 2
	                        , Object_member     // 3
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
