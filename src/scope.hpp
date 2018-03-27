// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_SCOPE_HPP_
#define ASTERIA_SCOPE_HPP_

#include "fwd.hpp"
#include "type_tuple.hpp"

namespace Asteria {

class Scope {
private:
	const std::weak_ptr<Scope> m_weak_parent_opt;

	Value_ptr_map<std::string, Reference> m_local_variables;

public:
	explicit Scope(const std::shared_ptr<Scope> &parent_opt)
		: m_weak_parent_opt(parent_opt)
	{ }
	~Scope();

public:
	std::shared_ptr<Scope> get_parent_opt() const noexcept {
		return m_weak_parent_opt.lock();
	}

	bool has_local_variable(const std::string &key) const noexcept;
	Reference get_local_variable(const std::string &key) const;
	void set_local_variable(const std::string &key, Reference &&reference);
	bool remove_local_variable(const std::string &key) noexcept;
	void clear_local_variables() noexcept;

	Reference get_variable_recursive() const;
};

}

#endif
