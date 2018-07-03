// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIABLE_HPP_
#define ASTERIA_VARIABLE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Variable {
private:
	Vp<Value> m_value_opt;
	bool m_immutable;

public:
	explicit Variable(bool immutable = false)
		: m_value_opt(), m_immutable(immutable)
	{ }
	Variable(Vp<Value> &&value_opt, bool immutable = false)
		: m_value_opt(std::move(value_opt)), m_immutable(immutable)
	{ }
	~Variable();

	Variable(const Variable &) = delete;
	Variable & operator=(const Variable &) = delete;

private:
	ROCKET_NORETURN void do_throw_immutable() const;

public:
	Vp_ref<const Value> get_value_opt() const noexcept {
		return m_value_opt;
	}
	std::reference_wrapper<Vp<Value>> mutate_value(){
		const auto immutable = is_immutable();
		if(immutable){
			do_throw_immutable();
		}
		return std::ref(m_value_opt);
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
