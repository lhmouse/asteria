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
	void (*m_target)(Vp<Reference> &, Vp<Reference> &&, Vector<Vp<Reference>> &&);

public:
	Slim_function(Cow_string description, void (*target)(Vp<Reference> &, Vp<Reference> &&, Vector<Vp<Reference>> &&))
		: m_description(std::move(description)), m_target(target)
	{ }
	~Slim_function() override;

public:
	Cow_string describe() const override;
	void invoke(Vp<Reference> &result_out, Vp<Reference> &&this_opt, Vector<Vp<Reference>> &&args) const override;
};

}

#endif
