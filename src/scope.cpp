// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "scope.hpp"
#include "variable.hpp"
#include "reference.hpp"
#include "misc.hpp"

namespace Asteria {

Scope::~Scope(){
	//
}

Value_ptr<Variable> *Scope::try_get_local_variable_recursive(const std::string &key){
	auto scope_current = this;
	for(;;){
		const auto it = scope_current->m_local_variables.find(key);
		if(it != scope_current->m_local_variables.end()){
			return &(it->second);
		}
		scope_current = scope_current->m_parent_opt.get();
		if(!scope_current){
			return nullptr;
		}
	}
}

Value_ptr<Variable> &Scope::declare_local_variable(const std::string &key, Value_ptr<Variable> &&variable_opt){
	return m_local_variables[key] = std::move(variable_opt);
}
void Scope::clear_local_variables() noexcept {
	for(auto &pair : m_local_variables){
		pair.second = false;
	}
	m_local_variables.clear();
}

}
