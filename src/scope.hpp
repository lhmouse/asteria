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
	const Sptr<const Scope> m_parent_opt;

	Xptr_map<std::string, Reference> m_local_references;
	Sptr_vector<const Function_base> m_deferred_callbacks;

public:
	Scope(Type type, Sptr<const Scope> parent_opt)
		: m_type(type), m_parent_opt(std::move(parent_opt))
		, m_local_references(), m_deferred_callbacks()
	{ }
	~Scope();

	Scope(const Scope &) = delete;
	Scope &operator=(const Scope &) = delete;

public:
	Type get_type() const noexcept {
		return m_type;
	}
	Spcref<const Scope> get_parent_opt() const noexcept {
		return m_parent_opt;
	}

	Sptr<const Reference> get_local_reference_opt(const std::string &identifier) const noexcept;
	std::reference_wrapper<Xptr<Reference>> drill_for_local_reference(const std::string &identifier);

	void defer_callback(Sptr<const Function_base> &&callback);
};

extern Sptr<const Reference> get_local_reference_cascade_opt(Spcref<const Scope> scope_opt, const std::string &identifier);

extern void prepare_function_scope(Spcref<Scope> scope, Spcref<Recycler> recycler, const std::vector<Function_parameter> &parameters, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt);

}

#endif
