// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_HPP_
#define ASTERIA_REFERENCE_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Reference {
private:
	struct Dereference_once_result;

public:
	enum Type : unsigned {
		type_rvalue_generic        = 0,
		type_lvalue_generic        = 1,
		type_lvalue_array_element  = 2,
		type_lvalue_object_member  = 3,
	};
	struct Rvalue_generic {
		Sp<Variable> xvar_opt;
	};
	struct Lvalue_generic {
		Sp<Scoped_variable> scoped_var;
	};
	struct Lvalue_array_element {
		Sp<Variable> rvar;
		bool immutable;
		std::int64_t index_bidirectional;
	};
	struct Lvalue_object_member {
		Sp<Variable> rvar;
		bool immutable;
		std::string key;
	};
	using Types = Type_tuple< Rvalue_generic        // 0
	                        , Lvalue_generic        // 1
	                        , Lvalue_array_element  // 2
	                        , Lvalue_object_member  // 3
		>;

private:
	Sp<Recycler> m_recycler;
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Reference, ValueT)>
	Reference(Sp<Recycler> recycler, ValueT &&value)
		: m_recycler(std::move(recycler)), m_variant(std::forward<ValueT>(value))
	{ }

	Reference(Reference &&);
	Reference &operator=(Reference &&);
	~Reference();

	Reference(const Reference &) = delete;
	Reference &operator=(const Reference &) = delete;

private:
	Dereference_once_result do_dereference_once_opt(bool create_if_not_exist) const;

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
	}
	template<typename ExpectT>
	const ExpectT &get() const {
		return boost::get<ExpectT>(m_variant);
	}
	template<typename ExpectT>
	ExpectT &get(){
		return boost::get<ExpectT>(m_variant);
	}
	template<typename ValueT>
	void set(ValueT &&value){
		m_variant = std::forward<ValueT>(value);
	}

	Sp<Variable> load_opt() const;
	void store(Stored_value &&value) const;
	Value_ptr<Variable> extract_opt();
};

}

#endif
