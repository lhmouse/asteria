// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "scope.hpp"
#include "utilities.hpp"

namespace Asteria {

Scope::~Scope(){
	clear_local_references();
}

Sptr<const Reference> Scope::get_local_reference_opt(const std::string &identifier) const noexcept {
	auto it = m_local_references.find(identifier);
	if(it == m_local_references.end()){
		return nullptr;
	}
	return it->second;
}
std::reference_wrapper<Xptr<Reference>> Scope::drill_for_local_reference(const std::string &identifier){
	auto it = m_local_references.find(identifier);
	if(it == m_local_references.end()){
		ASTERIA_DEBUG_LOG("Creating local reference: identifier = ", identifier);
		it = m_local_references.emplace(identifier, nullptr).first;
	}
	return it->second;
}
void Scope::clear_local_references() noexcept {
	decltype(m_local_references) local_references;
	local_references.swap(m_local_references);
}

Sptr<const Reference> get_local_reference_cascade(Spref<const Scope> scope_opt, const std::string &identifier){
	auto scope = scope_opt;
	for(;;){
		if(!scope){
			ASTERIA_THROW_RUNTIME_ERROR("Undeclared identifier `", identifier, "`");
		}
		auto local_ref = scope->get_local_reference_opt(identifier);
		if(local_ref){
			return std::move(local_ref);
		}
		scope = scope->get_parent_opt();
	}
}

}
