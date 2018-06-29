// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PARAMETER_HPP_
#define ASTERIA_PARAMETER_HPP_

#include "fwd.hpp"

namespace Asteria {

class Parameter {
private:
	Cow_string m_id;
	Sp<const Value> m_def_arg_opt;

public:
	Parameter(Cow_string id, Sp<const Value> def_arg_opt)
		: m_id(std::move(id)), m_def_arg_opt(std::move(def_arg_opt))
	{ }
	Parameter(const Parameter &) noexcept;
	Parameter & operator=(const Parameter &) noexcept;
	Parameter(Parameter &&) noexcept;
	Parameter & operator=(Parameter &&) noexcept;
	~Parameter();

public:
	const Cow_string & get_id() const {
		return m_id;
	}
	const Sp<const Value> & get_default_argument_opt() const {
		return m_def_arg_opt;
	}
};

extern void prepare_function_arguments(Vector<Vp<Reference>> &args_inout, const Vector<Parameter> &params);

}

#endif
