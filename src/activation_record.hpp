// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ACTIVATION_RECORD_HPP_
#define ASTERIA_ACTIVATION_RECORD_HPP_

#include <boost/container/flat_map.hpp>
#include <memory>
#include <string>

namespace Asteria {

class Variable;

class ActivationRecord {
private:
	const std::string m_name;
	const std::shared_ptr<ActivationRecord> m_parent;

	boost::container::flat_map<std::string, std::shared_ptr<Variable>> m_variables;

public:
	ActivationRecord(std::string name, std::shared_ptr<ActivationRecord> parent);
	~ActivationRecord();

	ActivationRecord(const ActivationRecord &) = delete;
	ActivationRecord &operator=(const ActivationRecord &) = delete;

public:
	const std::string &get_name() const noexcept {
		return m_name;
	}
	const std::shared_ptr<ActivationRecord> &get_parent() const noexcept {
		return m_parent;
	}

	bool has_variable(const std::string &id) const noexcept;
	std::shared_ptr<const Variable> get_variable(const std::string &id) const noexcept;
	std::shared_ptr<Variable> get_variable(const std::string &id) noexcept;
	std::shared_ptr<Variable> set_variable(const std::string &id, const std::shared_ptr<Variable> &variable_new);
	void clear_variables() noexcept;
};

}

#endif
