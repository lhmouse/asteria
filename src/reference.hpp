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
		Sptr<const Variable> variable_opt;
	};
	struct S_rvalue_dynamic {
		Xptr<Variable> variable_opt;
	};
	struct S_lvalue_scoped_variable {
		Sptr<Scoped_variable> scoped_variable;
	};
	struct S_lvalue_array_element {
		Sptr<Variable> variable;
		bool immutable;
		std::int64_t index_bidirectional;
	};
	struct S_lvalue_object_member {
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

// This function returns a read-only pointer.
extern Sptr<const Variable> read_reference_opt(bool *immutable_out_opt, Spref<const Reference> reference_opt);
// This function returns the contents of the variable before the call.
extern Sptr<Variable> write_reference(Spref<Reference> reference_opt, Spref<Recycler> recycler, Stored_value &&value_opt);
// This function returns the contents of the variable before the call.
// This function takes an rvalue of `Xptr<Reference>` to allow moving an dynamic rvalue efficiently.
// If you do not have an `Xptr` but an `Sptr`, use the following code to copy through the reference:
//   `copy_variable_recursive(variable_out, recycler, read_reference_opt(nullptr, reference_opt))`
extern Sptr<Variable> set_variable_using_reference(Xptr<Variable> &variable_out, Spref<Recycler> recycler, Xptr<Reference> &&reference_opt);

}

#endif
