// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_HPP_
#define ASTERIA_REFERENCE_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Reference : Deleted_copy {
public:
	enum Type : unsigned {
		type_rvalue_generic        = 0,
		type_lvalue_generic        = 1,
		type_lvalue_array_element  = 2,
		type_lvalue_object_member  = 3,
	};
	struct Rvalue_generic {
		Shared_ptr<Variable> value_opt;
	};
	struct Lvalue_generic {
		Shared_ptr<Named_variable> named_variable;
	};
	struct Lvalue_array_element {
		Shared_ptr<Variable> variable;
		std::int64_t index_bidirectional;
	};
	struct Lvalue_object_member {
		Shared_ptr<Variable> variable;
		std::string key;
	};
	using Types = Type_tuple< Rvalue_generic        // 0
	                        , Lvalue_generic        // 1
	                        , Lvalue_array_element  // 2
	                        , Lvalue_object_member  // 3
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Reference, ValueT)>
	Reference(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }

private:
	std::tuple<Shared_ptr<Variable>, Value_ptr<Variable> *> do_dereference_once_opt(bool create_if_not_exist) const;

public:
	Shared_ptr<Variable> load_opt() const;
	void store(const Shared_ptr<Recycler> &recycler, Stored_value &&value) const;
	Value_ptr<Variable> extract_opt(const Shared_ptr<Recycler> &recycler);
};

}

#endif
