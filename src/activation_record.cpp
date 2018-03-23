// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "activation_record.hpp"
#include "misc.hpp"

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
	bool result = false;
	const auto it = m_objects.find(id);
	if(it != m_objects.end()){
		result = true;
	}
	return result;
}
std::shared_ptr<const Object> ActivationRecord::get_object(const std::string &id) const noexcept {
	std::shared_ptr<const Object> object_old;
	const auto it = m_objects.find(id);
	if(it != m_objects.end()){
		object_old = it->second;
	}
	return object_old;
}
std::shared_ptr<Object> ActivationRecord::get_object(const std::string &id) noexcept {
	std::shared_ptr<Object> object_old;
	const auto it = m_objects.find(id);
	if(it != m_objects.end()){
		object_old = it->second;
	}
	return object_old;
}
std::shared_ptr<Object> ActivationRecord::set_object(const std::string &id, const std::shared_ptr<Object> &object_new){
	std::shared_ptr<Object> object_old;
	const auto it = m_objects.find(id);
	if(object_new && (it != m_objects.end())){
		// Replace the old object with the new one.
		object_old = std::move(it->second);
		it->second = object_new;
	} else if(object_new){
		// Insert the new object.
		m_objects.emplace(id, object_new);
	} else if(it != m_objects.end()){
		// Erase the old object.
		object_old = std::move(it->second);
		m_objects.erase(it);
	}
	return object_old;
}

}
