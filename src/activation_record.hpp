// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ACTIVATION_RECORD_HPP_
#define ASTERIA_ACTIVATION_RECORD_HPP_

#include <boost/container/flat_map.hpp>
#include <memory>
#include <string>

namespace Asteria {

class Object;

class ActivationRecord {
private:
	const std::string &m_name;
	const std::shared_ptr<ActivationRecord> m_parent;

	boost::container::flat_map<std::string, std::shared_ptr<Object>> m_objects;

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

	bool has_object(const std::string &id) const noexcept;
	std::shared_ptr<const Object> get_object(const std::string &id) const noexcept;
	std::shared_ptr<Object> get_object(const std::string &id) noexcept;
	std::shared_ptr<Object> set_object(const std::string &id, const std::shared_ptr<Object> &object_new);
};

}

#endif
