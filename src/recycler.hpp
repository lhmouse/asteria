// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RECYCLER_HPP_
#define ASTERIA_RECYCLER_HPP_

#include "fwd.hpp"

namespace Asteria {

class Recycler {
public:
	enum : std::size_t {
		defragmentation_threshold_initial    = 200,
		defragmentation_threshold_increment  = 100,
	};

private:
	boost::container::vector<Wptr<Variable>> m_weak_variables;
	std::size_t m_defragmentation_threshold;

public:
	Recycler()
		: m_weak_variables(), m_defragmentation_threshold(defragmentation_threshold_initial)
	{ }
	~Recycler();

	Recycler(const Recycler &) = delete;
	Recycler &operator=(const Recycler &) = delete;

public:
	Xptr<Variable> set_variable_opt(Xptr<Variable> &variable_out, Stored_value &&value_opt);
	void defragment_automatic() noexcept;
	void clear_variables() noexcept;
};

}

#endif
