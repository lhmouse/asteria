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

	struct Direct_reference {
		std::shared_ptr<Variable> variable_opt;
	};
	struct Array_element {
		std::shared_ptr<Variable> variable_opt;
		std::int64_t index_bidirectional;
	};
	struct Object_member {
		std::shared_ptr<Variable> variable_opt;
		std::string key;
	};

	using Types = Type_tuple< Direct_reference  // 0
	                        , Array_element     // 1
	                        , Object_member     // 2
		>;

private:
	Types::rebound_variant m_variant;
	bool m_immutable;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Reference, ValueT)>
	Reference(ValueT &&value, bool immutable = false)
		: m_variant(std::forward<ValueT>(value)), m_immutable(immutable)
	{ }
	~Reference();

public:
	bool is_immutable() const noexcept {
		return m_immutable;
	}
	void set_immutable(bool immutable = true) noexcept {
		m_immutable = immutable;
	}

	std::shared_ptr<const Variable> load() const;
	Value_ptr<Variable> &store(Value_ptr<Variable> &&new_value);

	Value_ptr<Variable> copy() const;
};

}

#endif
