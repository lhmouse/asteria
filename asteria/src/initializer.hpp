// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INITIALIZER_HPP_
#define ASTERIA_INITIALIZER_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Initializer {
public:
	enum Type : signed char {
		type_assignment_init      = 0,
		type_bracketed_init_list  = 1,
		type_braced_init_list     = 2,
	};
	struct S_assignment_init {
		Vp<Expression> expr;
	};
	struct S_bracketed_init_list {
		Vector<Vp<Initializer>> elems;
	};
	struct S_braced_init_list {
		Dictionary<Vp<Initializer>> key_values;
	};
	using Variant = rocket::variant<ASTERIA_CDR(void
		, S_assignment_init      // 0
		, S_bracketed_init_list  // 1
		, S_braced_init_list     // 2
	)>;

private:
	Variant m_variant;

public:
	template<typename CandidateT, ASTERIA_ENABLE_IF_ACCEPTABLE_BY_VARIANT(CandidateT, Variant)>
	Initializer(CandidateT &&cand)
		: m_variant(std::forward<CandidateT>(cand))
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

extern void bind_initializer(Vp<Initializer> &bound_init_out, Sp_ref<const Initializer> init_opt, Sp_ref<const Scope> scope);
extern void evaluate_initializer(Vp<Reference> &result_out, Sp_ref<Recycler> recycler_out, Sp_ref<const Initializer> init_opt, Sp_ref<const Scope> scope);

}

#endif
