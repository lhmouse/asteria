// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_HPP_
#define ASTERIA_REFERENCE_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Reference {
	friend Stored_reference;

public:
	enum Type : unsigned {
		type_null             = -1u,
		type_constant         =  0,
		type_temporary_value  =  1,
		type_variable         =  2,
		type_array_element    =  3,
		type_object_member    =  4,
	};
	struct S_constant {
		Sptr<const Value> source_opt;
	};
	struct S_temporary_value {
		Xptr<Value> value_opt;
	};
	struct S_variable {
		Sptr<Variable> variable;
	};
	struct S_array_element {
		Xptr<Reference> parent_opt;
		Signed_integer index;
	};
	struct S_object_member {
		Xptr<Reference> parent_opt;
		Cow_string key;
	};
	using Variant = rocket::variant<ASTERIA_CDR(void
		, S_constant         //  0
		, S_temporary_value  //  1
		, S_variable         //  2
		, S_array_element    //  3
		, S_object_member    //  4
	)>;

private:
	Variant m_variant;

public:
	template<typename CandidateT, ASTERIA_UNLESS_IS_BASE_OF(Reference, CandidateT)>
	Reference(CandidateT &&candidate)
		: m_variant(std::forward<CandidateT>(candidate))
	{ }
	~Reference();

	Reference(const Reference &) = delete;
	Reference & operator=(const Reference &) = delete;

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT * get_opt() const noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	ExpectT * get_opt() noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT & get() const {
		return m_variant.get<ExpectT>();
	}
	template<typename ExpectT>
	ExpectT & get(){
		return m_variant.get<ExpectT>();
	}
	template<typename CandidateT>
	void set(CandidateT &&candidate){
		m_variant = std::forward<CandidateT>(candidate);
	}
};

extern Reference::Type get_reference_type(Spparam<const Reference> reference_opt) noexcept;

extern void dump_reference(std::ostream &os, Spparam<const Reference> reference_opt, unsigned indent_next = 0, unsigned indent_increment = 2);
extern std::ostream & operator<<(std::ostream &os, const Sptr_formatter<Reference> &reference_fmt);

extern void copy_reference(Xptr<Reference> &reference_out, Spparam<const Reference> source_opt);
extern void move_reference(Xptr<Reference> &reference_out, Xptr<Reference> &&source_opt);

extern Sptr<const Value> read_reference_opt(Spparam<const Reference> reference_opt);
extern std::reference_wrapper<Xptr<Value>> drill_reference(Spparam<const Reference> reference_opt);

// If you do not have an `Xptr<Reference>` but an `Sptr<const Reference>`, use the following code to copy the value through the reference:
//   `copy_value(value_out, recycler, read_reference_opt(reference_opt))`
extern void extract_value_from_reference(Xptr<Value> &value_out, Spparam<Recycler> recycler, Xptr<Reference> &&reference_opt);

// If the reference is a temporary value, convert it to a value, allowing further modification to it.
extern void materialize_reference(Xptr<Reference> &reference_inout_opt, Spparam<Recycler> recycler, bool constant);

}

#endif
