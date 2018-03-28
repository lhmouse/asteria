// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SCOPE_HPP_
#define ASTERIA_SCOPE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Scope {
private:
	const Shared_ptr<Scope> m_parent_opt;

	boost::container::flat_map<std::string, Shared_ptr<Named_variable>> m_local_variables;

public:
	explicit Scope(Shared_ptr<Scope> parent_opt)
		: m_parent_opt(std::move(parent_opt))
		, m_local_variables()
	{ }
	~Scope();

	ASTERIA_FORBID_COPY(Scope)
	ASTERIA_FORBID_MOVE(Scope)

public:
	const Shared_ptr<Scope> &get_parent_opt() const noexcept {
		return m_parent_opt;
	}

	Shared_ptr<Named_variable> get_variable_recursive_opt(const std::string &key) const noexcept;
	Shared_ptr<Named_variable> declare_local_variable(const std::string &key);
	void clear_local_variables() noexcept;
};

}

#endif
