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
		type_null                    = -1u,
		type_rvalue_static           =  0,
		type_rvalue_dynamic          =  1,
		type_lvalue_scoped_variable  =  2,
		type_lvalue_array_element    =  3,
		type_lvalue_object_member    =  4,
	};
	struct S_rvalue_static {
		Sptr<Recycler> recycler;
		Sptr<const Variable> variable_opt;
	};
	struct S_rvalue_dynamic {
		Sptr<Variable> variable_opt;
	};
	struct S_lvalue_scoped_variable {
		Sptr<Recycler> recycler;
		Sptr<Scoped_variable> scoped_variable;
	};
	struct S_lvalue_array_element {
		Sptr<Recycler> recycler;
		Sptr<Variable> variable;
		bool immutable;
		std::int64_t index_bidirectional;
	};
	struct S_lvalue_object_member {
		Sptr<Recycler> recycler;
		Sptr<Variable> variable;
		bool immutable;
		std::string key;
	};
	using Types = Type_tuple< S_rvalue_static           // 0
	                        , S_rvalue_dynamic          // 1
	                        , S_lvalue_scoped_variable  // 2
	                        , S_lvalue_array_element    // 3
	                        , S_lvalue_object_member    // 4
		>;

private:
	Types::rebind_as_variant m_variant;

public:
	template<typename ValueT, ASTERIA_UNLESS_IS_BASE_OF(Reference, ValueT)>
	Reference(ValueT &&value)
		: m_variant(std::forward<ValueT>(value))
	{ }
	Reference(Reference &&);
	Reference &operator=(Reference &&);
	~Reference();

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.which());
	}
	template<typename ExpectT>
	const ExpectT *get_opt() const noexcept {
		return boost::get<ExpectT>(&m_variant);
	}
	template<typename ExpectT>
	ExpectT *get_opt() noexcept {
		return boost::get<ExpectT>(&m_variant);
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
};

extern Reference::Type get_reference_type(Spref<const Reference> reference_opt) noexcept;

extern Sptr<const Variable> read_reference_opt(Spref<const Reference> reference_opt);
extern void write_reference(Spref<Reference> reference_opt, Stored_value &&value_opt);
extern Xptr<Variable> extract_variable_from_reference_opt(Xptr<Reference> &&reference_opt);

}

#endif
