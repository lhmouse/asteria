// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "scope.hpp"
#include "reference.hpp"
#include "utilities.hpp"

namespace Asteria {

Scope::~Scope(){
	clear_local_references();
}

Sptr<Reference> Scope::get_local_reference(const std::string &identifier) const noexcept {
	auto it = m_local_references.find(identifier);
	if(it == m_local_references.end()){
		return nullptr;
	}
	return it->second;
}
void Scope::set_local_reference(const std::string &identifier, Xptr<Reference> &&reference_opt){
	auto it = m_local_references.find(identifier);
	if(it == m_local_references.end()){
		ASTERIA_DEBUG_LOG("Creating local reference: identifier = ", identifier);
		it = m_local_references.emplace(identifier, nullptr).first;
	}
	it->second = std::move(reference_opt);
}
void Scope::clear_local_references() noexcept {
	m_local_references.clear();
}

Sptr<Reference> get_reference_recursive_opt(Spref<const Scope> scope_opt, const std::string &identifier) noexcept {
	for(auto scope = scope_opt; scope; scope = scope->get_parent_opt()){
		auto reference = scope->get_local_reference(identifier);
		if(reference){
			return std::move(reference);
		}
	}
	return nullptr;
}

}
