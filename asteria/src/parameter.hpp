// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PARAMETER_HPP_
#define ASTERIA_PARAMETER_HPP_

#include "fwd.hpp"

namespace Asteria {

class Parameter {
private:
	C_cow_string m_identifier;
	Sptr<const Variable> m_default_argument_opt;

public:
	Parameter(C_cow_string identifier, Sptr<const Variable> default_argument_opt)
		: m_identifier(std::move(identifier)), m_default_argument_opt(std::move(default_argument_opt))
	{ }
	Parameter(Parameter &&) noexcept;
	Parameter & operator=(Parameter &&) noexcept;
	~Parameter();

public:
	const C_cow_string & get_identifier() const {
		return m_identifier;
	}
	const Sptr<const Variable> & get_default_argument_opt() const {
		return m_default_argument_opt;
	}
};

extern void prepare_function_arguments(Xptr_vector<Reference> &arguments_inout, const Sptr_vector<const Parameter> &parameters_opt);

}

#endif
