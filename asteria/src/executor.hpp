// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXECUTOR_HPP_
#define ASTERIA_EXECUTOR_HPP_

#include "fwd.hpp"

namespace Asteria {

class Executor {
private:
	Sptr<Recycler> m_recycler_opt;
	Sptr<Scope> m_root_scope_opt;

	Xptr<Block> m_code_opt;

public:
	Executor()
		: m_recycler_opt(), m_root_scope_opt()
	{ }
	~Executor();

	Executor(const Executor &) = delete;
	Executor & operator=(const Executor &) = delete;

private:
	const Sptr<Recycler> & do_get_recycler();
	const Sptr<Scope> & do_get_root_scope();

public:
	std::shared_ptr<Local_variable> set_root_variable(const D_string &identifier, Stored_value &&value, bool constant = false);
	std::shared_ptr<Local_variable> set_root_constant(const D_string &identifier, Stored_value &&value);
	std::shared_ptr<Local_variable> set_root_function(const D_string &identifier, Sptr<const Function_base> &&func);
	std::shared_ptr<Local_variable> set_root_slim_function(const D_string &identifier, D_string description, Function_base_prototype *target);

	void reset() noexcept;
};

}

#endif
