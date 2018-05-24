// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SLIM_FUNCTION_HPP_
#define ASTERIA_SLIM_FUNCTION_HPP_

#include "fwd.hpp"
#include "function_base.hpp"

namespace Asteria {

class Slim_function : public Function_base {
private:
	String m_description;
	Function_base_prototype *m_target;

public:
	Slim_function(String description, Function_base_prototype *target)
		: m_description(std::move(description)), m_target(target)
	{ }
	~Slim_function() override;

public:
	String describe() const override;
	void invoke(Xptr<Reference> &result_out, Spparam<Recycler> recycler, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt) const override;
};

}

#endif
