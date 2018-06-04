// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Variable {
private:
	Xptr<Value> m_value_opt;
	bool m_constant;

public:
	explicit Variable(bool constant = false)
		: m_value_opt(), m_constant(constant)
	{ }
	Variable(Xptr<Value> &&value_opt, bool constant = false)
		: m_value_opt(std::move(value_opt)), m_constant(constant)
	{ }
	~Variable();

	Variable(const Variable &) = delete;
	Variable & operator=(const Variable &) = delete;

private:
	ROCKET_NORETURN void do_throw_constant() const;

public:
	Sptr<const Value> get_value_opt() const noexcept {
		return m_value_opt;
	}
	std::reference_wrapper<Xptr<Value>> drill_for_value(){
		const auto constant = is_constant();
		if(constant){
			do_throw_constant();
		}
		return std::ref(m_value_opt);
	}

	bool is_constant() const noexcept {
		return m_constant;
	}
	void set_constant(bool constant = true) noexcept {
		m_constant = constant;
	}
};

}

#endif
