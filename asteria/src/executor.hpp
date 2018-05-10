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
		//
	{ }
	~Executor();

	Executor(const Executor &) = delete;
	Executor & operator=(const Executor &) = delete;

private:
	const Sptr<Recycler> & do_get_recycler();
	const Sptr<Scope> & do_get_root_scope();

public:
	void reset() noexcept;

	void set_root_variable(const std::string &identifier, Stored_value &&value, bool constant = false);
	void set_root_constant(const std::string &identifier, Stored_value &&value);
	void set_root_function(const std::string &identifier, Sptr<const Function_base> &&func);
	void set_root_function(const std::string &identifier, Function_base_prototype *fptr);

	
};

}

#endif
