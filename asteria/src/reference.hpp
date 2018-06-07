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
	enum Type : signed char {
		type_null             = -1,
		type_constant         =  0,
		type_temporary_value  =  1,
		type_variable         =  2,
		type_array_element    =  3,
		type_object_member    =  4,
	};
	struct S_constant {
		Sp<const Value> src_opt;
	};
	struct S_temporary_value {
		Vp<Value> value_opt;
	};
	struct S_variable {
		Sp<Variable> variable;
	};
	struct S_array_element {
		Vp<Reference> parent_opt;
		Signed_integer index;
	};
	struct S_object_member {
		Vp<Reference> parent_opt;
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
	template<typename CandidateT, ASTERIA_ENABLE_IF_ACCEPTABLE_BY_VARIANT(CandidateT, Variant)>
	Reference(CandidateT &&cand)
		: m_variant(std::forward<CandidateT>(cand))
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
	void set(CandidateT &&cand){
		m_variant = std::forward<CandidateT>(cand);
	}
};

extern Reference::Type get_reference_type(Spr<const Reference> reference_opt) noexcept;

extern void dump_reference(std::ostream &os, Spr<const Reference> reference_opt, unsigned indent_next = 0, unsigned indent_increment = 2);
extern std::ostream & operator<<(std::ostream &os, const Sp_formatter<Reference> &reference_fmt);

extern void copy_reference(Vp<Reference> &reference_out, Spr<const Reference> src_opt);
extern void move_reference(Vp<Reference> &reference_out, Vp<Reference> &&src_opt);

extern Sp<const Value> read_reference_opt(Spr<const Reference> reference_opt);
extern std::reference_wrapper<Vp<Value>> drill_reference(Spr<const Reference> reference_opt);

// If you do not have an `Vp<Reference>` but an `Sp<const Reference>`, use the following code to copy the value through the reference:
//   `copy_value(value_out, recycler, read_reference_opt(reference_opt))`
extern void extract_value_from_reference(Vp<Value> &value_out, Spr<Recycler> recycler, Vp<Reference> &&reference_opt);

// If the reference is a temporary value, convert it to a value, allowing further modification to it.
extern void materialize_reference(Vp<Reference> &reference_inout_opt, Spr<Recycler> recycler, bool immutable);

}

#endif
