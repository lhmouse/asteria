// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "scope.hpp"
#include "variable.hpp"

namespace Asteria {

Scope::~Scope(){
	//
}

std::shared_ptr<Value_ptr<Variable>> Scope::get_variable_recursive_opt(const std::string &key) const noexcept {
	auto scope = this;
	for(;;){
		const auto it = scope->m_local_variables.find(key);
		if(it != scope->m_local_variables.end()){
			return it->second;
		}
		scope = scope->m_parent_opt.get();
		if(!scope){
			return nullptr;
		}
	}
}
std::shared_ptr<Value_ptr<Variable>> Scope::declare_local_variable(const std::string &key){
	auto it = m_local_variables.find(key);
	if(it == m_local_variables.end()){
		auto variable_ptr = std::make_shared<Value_ptr<Variable>>();
		it = m_local_variables.emplace(key, std::move(variable_ptr)).first;
	}
	return it->second;
}
void Scope::clear_local_variables() noexcept {
	for(const auto &pair : m_local_variables){
		auto &variable_ptr = *(pair.second);
		if(!variable_ptr){
			continue;
		}
		// Overwrite the value stored in this variable first.
		// This breaks circular references, if any.
		variable_ptr->set(false);
		variable_ptr = nullptr;
	}
	m_local_variables.clear();
}

}
