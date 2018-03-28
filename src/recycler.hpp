// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RECYCLER_HPP_
#define ASTERIA_RECYCLER_HPP_

#include "fwd.hpp"

namespace Asteria {

class Recycler : Deleted_move {
private:
	boost::container::vector<std::weak_ptr<Variable>> m_weak_variables;

public:
	Recycler()
		: m_weak_variables()
	{ }
	~Recycler();

private:
	template<typename ValueT>
	void do_set_variable_explicit(Value_ptr<Variable> &variable, ValueT &&value);

public:
	Value_ptr<Variable> create_variable_opt(Nullable_value &&value_opt);
	void set_variable(Value_ptr<Variable> &variable, Nullable_value &&value_opt);
	void clear_variables() noexcept;
};

}

#endif
