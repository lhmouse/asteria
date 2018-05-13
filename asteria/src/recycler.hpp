// This file is part of Asteria.
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
	Wptr_vector<Variable> m_weak_variables;
	std::size_t m_defragmentation_threshold = defragmentation_threshold_initial;

public:
	Recycler()
		//
	{ }
	~Recycler();

	Recycler(const Recycler &) = delete;
	Recycler & operator=(const Recycler &) = delete;

private:
	void do_purge_variables() noexcept;

public:
	void adopt_variable(Sparg<Variable> variable_opt);
	void defragment(bool aggressive = false) noexcept;
};

}

#endif
