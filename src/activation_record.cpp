// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "activation_record.hpp"
#include "misc.hpp"

namespace Asteria {

ActivationRecord::ActivationRecord(std::string name, std::shared_ptr<ActivationRecord> parent)
	: m_name(std::move(name)), m_parent(std::move(parent))
{
	ASTERIA_DEBUG_LOG("ActivationRecord constructor: this = ", this, ", name = ", get_name());
}
ActivationRecord::~ActivationRecord(){
	ASTERIA_DEBUG_LOG("ActivationRecord destructor: this = ", this, ", name = ", get_name());
}

bool ActivationRecord::has_variable(const std::string &id) const noexcept {
	bool result = false;
	const auto it = m_variables.find(id);
	if(it != m_variables.end()){
		result = true;
	}
	return result;
}
std::shared_ptr<const Variable> ActivationRecord::get_variable(const std::string &id) const noexcept {
	std::shared_ptr<const Variable> variable_old;
	const auto it = m_variables.find(id);
	if(it != m_variables.end()){
		variable_old = it->second;
	}
	return variable_old;
}
std::shared_ptr<Variable> ActivationRecord::get_variable(const std::string &id) noexcept {
	std::shared_ptr<Variable> variable_old;
	const auto it = m_variables.find(id);
	if(it != m_variables.end()){
		variable_old = it->second;
	}
	return variable_old;
}
std::shared_ptr<Variable> ActivationRecord::set_variable(const std::string &id, const std::shared_ptr<Variable> &variable_new){
	std::shared_ptr<Variable> variable_old;
	const auto it = m_variables.find(id);
	if(variable_new && (it != m_variables.end())){
		// Replace the old variable with the new one.
		variable_old = std::move(it->second);
		it->second = variable_new;
	} else if(variable_new){
		// Insert the new variable.
		m_variables.emplace(id, variable_new);
	} else if(it != m_variables.end()){
		// Erase the old variable.
		variable_old = std::move(it->second);
		m_variables.erase(it);
	}
	return variable_old;
}
void ActivationRecord::clear_variables() noexcept {
	m_variables.clear();
}

}
