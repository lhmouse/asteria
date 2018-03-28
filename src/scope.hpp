// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SCOPE_HPP_
#define ASTERIA_SCOPE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Scope {
private:
	const std::shared_ptr<Scope> m_parent_opt;

	// XXX Can we get rid of these pointer-to-pointers ??!
	boost::container::flat_map<std::string, std::shared_ptr<Value_ptr<Variable>>> m_local_variables;

public:
	explicit Scope(std::shared_ptr<Scope> parent_opt)
		: m_parent_opt(std::move(parent_opt))
	{ }
	~Scope();

public:
	const std::shared_ptr<Scope> &get_parent_opt() const noexcept {
		return m_parent_opt;
	}

	std::shared_ptr<Value_ptr<Variable>> try_get_local_variable_recursive(const std::string &key) const noexcept;
	std::shared_ptr<Value_ptr<Variable>> declare_local_variable(const std::string &key);
	void clear_local_variables() noexcept;
};

}

#endif
