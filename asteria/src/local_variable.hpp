// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LOCAL_VARIABLE_HPP_
#define ASTERIA_LOCAL_VARIABLE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Local_variable {
private:
	Xptr<Variable> m_variable_opt;
	bool m_constant;

public:
	explicit Local_variable(bool constant = false)
		: m_variable_opt(), m_constant(constant)
	{ }
	Local_variable(Xptr<Variable> &&variable_opt, bool constant = false)
		: m_variable_opt(std::move(variable_opt)), m_constant(constant)
	{ }
	~Local_variable();

	Local_variable(const Local_variable &) = delete;
	Local_variable & operator=(const Local_variable &) = delete;

private:
	ROCKET_NORETURN void do_throw_local_constant() const;

public:
	Sptr<const Variable> get_variable_opt() const noexcept {
		return m_variable_opt;
	}
	std::reference_wrapper<Xptr<Variable>> drill_for_variable(){
		const auto constant = is_constant();
		if(constant){
			do_throw_local_constant();
		}
		return std::ref(m_variable_opt);
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
