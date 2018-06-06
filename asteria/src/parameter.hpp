// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PARAMETER_HPP_
#define ASTERIA_PARAMETER_HPP_

#include "fwd.hpp"

namespace Asteria {

class Parameter {
private:
	Cow_string m_id;
	Sp<const Value> m_default_argument_opt;

public:
	Parameter(Cow_string id, Sp<const Value> default_argument_opt)
		: m_id(std::move(id)), m_default_argument_opt(std::move(default_argument_opt))
	{ }
	Parameter(Parameter &&) noexcept;
	Parameter & operator=(Parameter &&) noexcept;
	~Parameter();

public:
	const Cow_string & get_id() const {
		return m_id;
	}
	const Sp<const Value> & get_default_argument_opt() const {
		return m_default_argument_opt;
	}
};

extern void prepare_function_arguments(Vp_vector<Reference> &arguments_inout, const Sp_vector<const Parameter> &parameters_opt);

}

#endif
