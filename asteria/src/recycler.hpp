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
	Vector<Wp<Value>> m_weak_values;
	std::size_t m_defragmentation_threshold;

public:
	Recycler()
		: m_weak_values(), m_defragmentation_threshold(defragmentation_threshold_initial)
	{ }
	~Recycler();

	Recycler(const Recycler &) = delete;
	Recycler & operator=(const Recycler &) = delete;

private:
	void do_purge_values() noexcept;

public:
	void adopt_value(Sp_cref<Value> value_opt);
	void defragment(bool aggressive = false) noexcept;
};

}

#endif
