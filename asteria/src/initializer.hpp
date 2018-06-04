// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INITIALIZER_HPP_
#define ASTERIA_INITIALIZER_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Initializer {
public:
	enum Type : unsigned {
		type_null                 = -1u,
		type_assignment_init      =  0,
		type_bracketed_init_list  =  1,
		type_braced_init_list     =  2,
	};
	struct S_assignment_init {
		Vp<Expression> expression;
	};
	struct S_bracketed_init_list {
		Vp_vector<Initializer> elements;
	};
	struct S_braced_init_list {
		Vp_string_map<Initializer> key_values;
	};
	using Variant = rocket::variant<ASTERIA_CDR(void
		, S_assignment_init     // 0
		, S_bracketed_init_list // 1
		, S_braced_init_list    // 2
	)>;

private:
	Variant m_variant;

public:
	template<typename CandidateT, ASTERIA_UNLESS_IS_BASE_OF(Initializer, CandidateT)>
	Initializer(CandidateT &&candidate)
		: m_variant(std::forward<CandidateT>(candidate))
	{ }
	Initializer(Initializer &&) noexcept;
	Initializer & operator=(Initializer &&) noexcept;
	~Initializer();

public:
	Type get_type() const noexcept {
		return static_cast<Type>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT * get_opt() const noexcept {
		return m_variant.try_get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT & get() const {
		return m_variant.get<ExpectT>();
	}
};

extern Initializer::Type get_initializer_type(Spr<const Initializer> initializer_opt) noexcept;

extern void bind_initializer(Vp<Initializer> &bound_result_out, Spr<const Initializer> initializer_opt, Spr<const Scope> scope);
extern void evaluate_initializer(Vp<Reference> &reference_out, Spr<Recycler> recycler, Spr<const Initializer> initializer_opt, Spr<const Scope> scope);

}

#endif
