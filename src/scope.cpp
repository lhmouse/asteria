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

Sptr<Scoped_variable> Scope::get_variable_local_opt(const std::string &key) const noexcept {
	auto it = m_variables.find(key);
	if(it == m_variables.end()){
		return nullptr;
	}
	return it->second;
}
Sptr<Scoped_variable> Scope::declare_variable_local(const std::string &key){
	auto it = m_variables.find(key);
	if(it == m_variables.end()){
		ASTERIA_DEBUG_LOG("Creating local variable: key = ", key);
		auto xptr = create_shared<Scoped_variable>();
		it = m_variables.emplace(key, std::move(xptr)).first;
	}
	return it->second;
}
void Scope::clear_variables_local() noexcept {
	m_variables.clear();
}

Sptr<Scoped_variable> get_variable_recursive_opt(Spref<const Scope> scope_opt, const std::string &key) noexcept {
	for(auto scope = scope_opt.get(); scope; scope = scope->get_parent_opt().get()){
		auto scoped_var = scope->get_variable_local_opt(key);
		if(scoped_var){
			return std::move(scoped_var);
		}
	}
	return nullptr;
}

}
