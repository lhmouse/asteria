// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LOCAL_VARIABLE_HPP_
#define ASTERIA_LOCAL_VARIABLE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Local_variable {
private:
	Xptr<Variable> m_variable_opt;
	bool m_immutable;

public:
	explicit Local_variable(bool immutable = false)
		: m_variable_opt(), m_immutable(immutable)
	{ }
	Local_variable(Xptr<Variable> &&variable_opt, bool immutable = false)
		: m_variable_opt(std::move(variable_opt)), m_immutable(immutable)
	{ }
	~Local_variable();

	Local_variable(const Local_variable &) = delete;
	Local_variable &operator=(const Local_variable &) = delete;

private:
	__attribute__((__noreturn__)) void do_throw_immutable_local_variable() const;

public:
	Sptr<const Variable> get_variable_opt() const noexcept {
		return m_variable_opt;
	}
	std::reference_wrapper<Xptr<Variable>> drill_for_variable(){
		const auto immutable = is_immutable();
		if(immutable){
			do_throw_immutable_local_variable();
		}
		return std::ref(m_variable_opt);
	}

	bool is_immutable() const noexcept {
		return m_immutable;
	}
	void set_immutable(bool immutable = true) noexcept {
		m_immutable = immutable;
	}
};

}

#endif
