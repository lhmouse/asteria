// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SCOPE_HPP_
#define ASTERIA_SCOPE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Scope {
public:
	enum Type : unsigned {
		type_plain     = 0, // Created by a plain compound-statement.
		type_function  = 1, // Created by a call to a plain function, to the result of a lambda expression or to a defer-statement.
		type_switch    = 2, // Created by a switch-statement.
		type_loop      = 3, // Created by a do-while-statement, a while-statement, a for-statement or a foreach-statement.
	};

private:
	const Type m_type;
	const Shared_ptr<Scope> m_parent_opt;

	boost::container::flat_map<std::string, Shared_ptr<Named_variable>> m_variables;

public:
	Scope(Type type, Shared_ptr<Scope> parent_opt)
		: m_type(type), m_parent_opt(std::move(parent_opt))
		, m_variables()
	{ }
	~Scope();

	Scope(const Scope &) = delete;
	Scope &operator=(const Scope &) = delete;

public:
	Type get_type() const noexcept {
		return m_type;
	}
	Shared_ptr<Scope> get_parent_opt() const noexcept {
		return m_parent_opt;
	}

	Shared_ptr<Named_variable> get_variable_local_opt(const std::string &key) const noexcept;
	Shared_ptr<Named_variable> declare_variable_local(const std::string &key);
	void clear_variables_local() noexcept;
};

extern Shared_ptr<Named_variable> get_variable_recursive_opt(const std::shared_ptr<const Scope> &scope_opt, const std::string &key) noexcept;

}

#endif
