// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_INSTANTIATED_FUNCTION_HPP_

#include "fwd.hpp"
#include "function_base.hpp"

namespace Asteria {

class Instantiated_function : public Function_base {
private:
	const char * m_category;
	Cow_string m_source;

	Vector<Parameter> m_params;
	Sp<const Scope> m_bound_scope;
	Vp<Block> m_bound_body_opt;

public:
	Instantiated_function(const char *category, Cow_string source, Vector<Parameter> params, Sp<const Scope> bound_scope, Vp<Block> &&bound_body_opt)
		: m_category(category), m_source(std::move(source))
		, m_params(std::move(params)), m_bound_scope(std::move(bound_scope)), m_bound_body_opt(std::move(bound_body_opt))
	{ }
	~Instantiated_function() override;

public:
	Cow_string describe() const override;
	void invoke(Vp<Reference> &result_out, Spr<Recycler> recycler_out, Vp<Reference> &&this_opt, Vector<Vp<Reference>> &&args) const override;
};

}

#endif
