// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SIMPLE_FUNCTION_HPP_
#define ASTERIA_SIMPLE_FUNCTION_HPP_

#include "fwd.hpp"
#include "function_base.hpp"

namespace Asteria {

class Simple_function : public Function_base {
private:
	Function_base_prototype *m_target;

public:
	Simple_function(std::string description, Function_base_prototype *target)
		: Function_base(std::move(description))
		, m_target(target)
	{ }
	~Simple_function();

	Simple_function(const Simple_function &) = delete;
	Simple_function & operator=(const Simple_function &) = delete;

public:
	void invoke(Xptr<Reference> &result_out, Sparg<Recycler> recycler, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt) const override;
};

}

#endif
