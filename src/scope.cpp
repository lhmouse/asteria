// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "scope.hpp"
#include "variable.hpp"
#include "misc.hpp"

namespace Asteria {

Scope::~Scope(){
	clear_local_variables();
}

Shared_ptr<Named_variable> Scope::get_variable_recursive_opt(const std::string &key) const noexcept {
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
Shared_ptr<Named_variable> Scope::declare_local_variable(const std::string &key){
	auto it = m_local_variables.find(key);
	if(it == m_local_variables.end()){
		ASTERIA_DEBUG_LOG("Creating NEW local variable: key = ", key);
		auto named_variable = std::make_shared<Named_variable>();
		it = m_local_variables.emplace(key, std::move(named_variable)).first;
	}
	return it->second;
}
void Scope::clear_local_variables() noexcept {
	m_local_variables.clear();
}

}
