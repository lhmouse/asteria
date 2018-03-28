// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXCEPTION_HPP_
#define ASTERIA_EXCEPTION_HPP_

#include "fwd.hpp"
#include <exception>

namespace Asteria {

class Exception : public std::exception {
private:
	std::shared_ptr<Variable> m_variable_opt;

public:
	explicit Exception(std::shared_ptr<Variable> variable_opt)
		: m_variable_opt(std::move(variable_opt))
	{ }
	~Exception() override;

public:
	const char *what() const noexcept override {
		return "Asteria::Exception";
	}

	const std::shared_ptr<Variable> &get_variable_opt() const noexcept {
		return m_variable_opt;
	}
	void set_variable(std::shared_ptr<Variable> variable_opt) noexcept {
		m_variable_opt = std::move(variable_opt);
	}
};

}

#endif
