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
	boost::container::vector<std::weak_ptr<Variable>> m_weak_variables;
	std::size_t m_defragmentation_threshold;

public:
	Recycler()
		: m_weak_variables(), m_defragmentation_threshold(defragmentation_threshold_initial)
	{ }
	~Recycler();

	Recycler(const Recycler &) = delete;
	Recycler &operator=(const Recycler &) = delete;

public:
	Sptr<Variable> set_variable(Xptr<Variable> &variable_out_opt, Stored_value &&value_opt);
	Sptr<Variable> copy_variable_recursive(Xptr<Variable> &variable_out_opt, Spref<const Variable> source_opt);
	Sptr<Variable> initialize_variable_recursive(Xptr<Variable> &variable_out_opt, Spref<const Initializer> initializer_opt);
	void defragment_automatic() noexcept;
	void clear_variables() noexcept;
};

}

#endif
