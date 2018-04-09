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
	const Sptr<Scope> m_parent_opt;

	boost::container::flat_map<std::string, Xptr<Reference>> m_local_references;

public:
	Scope(Type type, Sptr<Scope> parent_opt)
		: m_type(type), m_parent_opt(std::move(parent_opt))
		, m_local_references()
	{ }
	~Scope();

	Scope(const Scope &) = delete;
	Scope &operator=(const Scope &) = delete;

public:
	Type get_type() const noexcept {
		return m_type;
	}
	Spref<Scope> get_parent_opt() const noexcept {
		return m_parent_opt;
	}

	Sptr<const Reference> get_local_reference_opt(const std::string &identifier) const noexcept;
	void set_local_reference(const std::string &identifier, Xptr<Reference> &&reference_opt);
	void clear_local_references() noexcept;
};

}

#endif
