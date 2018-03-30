// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "scope.hpp"
#include "variable.hpp"
#include "misc.hpp"

namespace Asteria {

Scope::~Scope(){
	clear_variables_local();
}

Sp<Named_variable> Scope::get_variable_local_opt(const std::string &key) const noexcept {
	auto it = m_variables.find(key);
	if(it == m_variables.end()){
		return nullptr;
	}
	return it->second;
}
Sp<Named_variable> Scope::declare_variable_local(const std::string &key){
	auto it = m_variables.find(key);
	if(it == m_variables.end()){
		ASTERIA_DEBUG_LOG("Creating local variable: key = ", key);
		auto xptr = create_shared<Named_variable>();
		it = m_variables.emplace(key, std::move(xptr)).first;
	}
	return it->second;
}
void Scope::clear_variables_local() noexcept {
	m_variables.clear();
}

Sp<Named_variable> get_variable_recursive_opt(Spref<const Scope> scope_opt, const std::string &key) noexcept {
	for(auto scope = scope_opt.get(); scope; scope = scope->get_parent_opt().get()){
		auto named_var = scope->get_variable_local_opt(key);
		if(named_var){
			return std::move(named_var);
		}
	}
	return nullptr;
}

}
