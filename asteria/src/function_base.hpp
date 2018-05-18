// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FUNCTION_BASE_HPP_
#define ASTERIA_FUNCTION_BASE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Function_base {
private:
	const std::string m_description;

public:
	explicit Function_base(std::string description)
		: m_description(std::move(description))
	{ }
	virtual ~Function_base();

	Function_base(const Function_base &) = delete;
	Function_base & operator=(const Function_base &) = delete;

public:
	const std::string & describe() const noexcept {
		return m_description;
	}

	virtual void invoke(Xptr<Reference> &result_out, Sparg<Recycler> recycler, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt) const = 0;
};

}

#endif
