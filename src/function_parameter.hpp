// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FUNCTION_PARAMETER_HPP_
#define ASTERIA_FUNCTION_PARAMETER_HPP_

#include "fwd.hpp"

namespace Asteria {

class Function_parameter {
private:
	std::string m_identifier;
	Sptr<const Variable> m_default_argument_opt;

public:
	Function_parameter(std::string identifier, Sptr<const Variable> default_argument_opt)
		: m_identifier(std::move(identifier)), m_default_argument_opt(std::move(default_argument_opt))
	{ }
	Function_parameter(Function_parameter &&) noexcept;
	Function_parameter &operator=(Function_parameter &&);
	~Function_parameter();

public:
	const std::string &get_identifier() const {
		return m_identifier;
	}
	const Sptr<const Variable> &get_default_argument_opt() const {
		return m_default_argument_opt;
	}
};

extern void prepare_function_arguments(Xptr_vector<Reference> &arguments_inout, const std::vector<Function_parameter> &parameters);

}

#endif
