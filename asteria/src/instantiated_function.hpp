// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_INSTANTIATED_FUNCTION_HPP_

#include "fwd.hpp"
#include "function_base.hpp"

namespace Asteria {

class Instantiated_function : public Function_base {
private:
	Sptr<const std::vector<Parameter>> m_parameters_opt;
	Sptr<const Scope> m_bound_scope;
	Xptr<Block> m_bound_body_opt;

public:
	Instantiated_function(String description, Sptr<const std::vector<Parameter>> parameters_opt, Sptr<const Scope> bound_scope, Xptr<Block> &&bound_body_opt)
		: Function_base(std::move(description))
		, m_parameters_opt(std::move(parameters_opt)), m_bound_scope(std::move(bound_scope)), m_bound_body_opt(std::move(bound_body_opt))
	{ }
	~Instantiated_function();

	Instantiated_function(const Instantiated_function &) = delete;
	Instantiated_function & operator=(const Instantiated_function &) = delete;

public:
	void invoke(Xptr<Reference> &result_out, Sparg<Recycler> recycler, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt) const override;
};

}

#endif
