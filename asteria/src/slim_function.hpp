// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SLIM_FUNCTION_HPP_
#define ASTERIA_SLIM_FUNCTION_HPP_

#include "fwd.hpp"
#include "function_base.hpp"

namespace Asteria {

class Slim_function : public Function_base {
private:
	Cow_string m_description;
	Function_prototype *m_target;

public:
	Slim_function(Cow_string description, Function_prototype *target)
		: m_description(std::move(description)), m_target(target)
	{ }
	~Slim_function() override;

public:
	Cow_string describe() const override;
	void invoke(Vp<Reference> &result_out, Spr<Recycler> recycler, Vp<Reference> &&this_opt, Vp_vector<Reference> &&arguments_opt) const override;
};

}

#endif
