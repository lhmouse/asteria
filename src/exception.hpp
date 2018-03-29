// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXCEPTION_HPP_
#define ASTERIA_EXCEPTION_HPP_

#include "fwd.hpp"
#include <exception>

namespace Asteria {

class Exception : public std::exception {
private:
	Shared_ptr<Variable> m_variable_opt;

public:
	explicit Exception(Shared_ptr<Variable> variable_opt)
		: m_variable_opt(std::move(variable_opt))
	{ }
	~Exception() override;

public:
	const char *what() const noexcept override {
		return "Asteria::Exception";
	}

	Shared_ptr<Variable> get_variable_opt() const noexcept {
		return m_variable_opt;
	}
	void set_variable(Shared_ptr<Variable> variable_opt) noexcept {
		m_variable_opt = std::move(variable_opt);
	}
	void clear_variable() noexcept {
		m_variable_opt = nullptr;
	}
};

}

#endif
