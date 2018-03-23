// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "activation_record.hpp"

namespace Asteria {

ActivationRecord::ActivationRecord(std::string name, std::shared_ptr<ActivationRecord> parent)
	: m_name(std::move(name)), m_parent(std::move(parent))
{
	DEBUG_PRINTF("ActivationRecord constructor: name = %s\n", m_name.c_str());
}
ActivationRecord::~ActivationRecord(){
	DEBUG_PRINTF("ActivationRecord destructor: name = %s\n", m_name.c_str());
}

bool ActivationRecord::has_object(const std::string &id) const noexcept {
	const auto it = m_objects.find(id);
	if(it == m_objects.end()){
		return false;
	}
	return true;
}
std::shared_ptr<const Object> ActivationRecord::get_object(const std::string &id) const noexcept {
	const auto it = m_objects.find(id);
	if(it == m_objects.end()){
		return nullptr;
	}
	return it->second;
}
std::shared_ptr<Object> ActivationRecord::get_object(const std::string &id) noexcept {
	const auto it = m_objects.find(id);
	if(it == m_objects.end()){
		return nullptr;
	}
	return it->second;
}
std::shared_ptr<Object> ActivationRecord::set_object(const std::string &id, const std::shared_ptr<Object> &object){
	std::shared_ptr<Object> old_object;
	const auto it = m_objects.find(id);
	if(it == m_objects.end()){
		if(object){
			// Inserts the new object.
			m_objects.emplace(id, object);
		} else {
			// No old object found. Nothing to erase.
		}
		return old_object;
	}
	old_object = std::move(it->second);
	if(object){
		// Replace the old object with the new one.
		it->second = object;
	} else {
		// Erase the old object.
		m_objects.erase(it);
	}
	return old_object;
}

}
