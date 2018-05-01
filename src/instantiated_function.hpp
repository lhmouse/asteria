// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_INSTANTIATED_FUNCTION_HPP_

#include "fwd.hpp"
#include "function_base.hpp"

namespace Asteria {

class Instantiated_function : public Function_base {
private:
	Sptr<const Scope> m_defined_in_scope;
	Sptr<const std::vector<Parameter>> m_parameters_opt;
	Xptr<Block> m_bound_body_opt;

public:
	Instantiated_function(Sptr<const Scope> defined_in_scope, Sptr<const std::vector<Parameter>> parameters_opt, Xptr<Block> &&bound_body_opt)
		: m_defined_in_scope(std::move(defined_in_scope)), m_parameters_opt(std::move(parameters_opt)), m_bound_body_opt(std::move(bound_body_opt))
	{ }
	~Instantiated_function();

	Instantiated_function(const Instantiated_function &) = delete;
	Instantiated_function &operator=(const Instantiated_function &) = delete;

public:
	const char *describe() const noexcept override;
	void invoke(Xptr<Reference> &result_out, Spcref<Recycler> recycler, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt) const override;
};

}

#endif
