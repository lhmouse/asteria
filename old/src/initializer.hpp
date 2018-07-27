// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INITIALIZER_HPP_
#define ASTERIA_INITIALIZER_HPP_

#include "fwd.hpp"
#include "rocket/variant.hpp"

namespace Asteria {

class Initializer {
public:
	enum Index : std::uint8_t {
		index_assignment_init      = 0,
		index_bracketed_init_list  = 1,
		index_braced_init_list     = 2,
	};
	struct S_assignment_init {
		Expression expr;
	};
	struct S_bracketed_init_list {
		Vector<Initializer> elems;
	};
	struct S_braced_init_list {
		Dictionary<Initializer> pairs;
	};
	using Variant = rocket::variant<ROCKET_CDR(
		, S_assignment_init      // 0
		, S_bracketed_init_list  // 1
		, S_braced_init_list     // 2
	)>;

private:
	Variant m_variant;

public:
	template<typename CandidateT, typename std::enable_if<std::is_constructible<Variant, CandidateT &&>::value>::type * = nullptr>
	Initializer(CandidateT &&cand)
		: m_variant(std::forward<CandidateT>(cand))
	{ }
	Initializer(Initializer &&) noexcept;
	Initializer & operator=(Initializer &&) noexcept;
	~Initializer();

public:
	Index which() const noexcept
	{
		return static_cast<Index>(m_variant.index());
	}
	template<typename ExpectT>
	const ExpectT * get_opt() const noexcept
	{
		return m_variant.get<ExpectT>();
	}
	template<typename ExpectT>
	const ExpectT & as() const
	{
		return m_variant.as<ExpectT>();
	}
};

extern Initializer bind_initializer(const Initializer &init, Sp_cref<const Scope> scope);
extern void evaluate_initializer(Vp<Reference> &result_out, const Initializer &init, Sp_cref<const Scope> scope);

}

#endif
