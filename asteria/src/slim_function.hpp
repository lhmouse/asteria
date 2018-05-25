// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SLIM_FUNCTION_HPP_
#define ASTERIA_SLIM_FUNCTION_HPP_

#include "fwd.hpp"
#include "function_base.hpp"

namespace Asteria {

class Slim_function : public Function_base {
private:
	C_cow_string m_description;
	C_function_prototype *m_target;

public:
	Slim_function(C_cow_string description, C_function_prototype *target)
		: m_description(std::move(description)), m_target(target)
	{ }
	~Slim_function() override;

public:
	C_cow_string describe() const override;
	void invoke(Xptr<Reference> &result_out, Spparam<Recycler> recycler, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt) const override;
};

}

#endif
