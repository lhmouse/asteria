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

Xptr<Reference> Scope::get_local_reference_opt(const std::string &identifier) const noexcept {
	auto it = m_local_references.find(identifier);
	if(it == m_local_references.end()){
		return nullptr;
	}
	return duplicate_reference_trivial(it->second);
}
void Scope::set_local_reference(const std::string &identifier, Xptr<Reference> &&reference_opt){
	auto it = m_local_references.find(identifier);
	if(it == m_local_references.end()){
		ASTERIA_DEBUG_LOG("Creating local reference: identifier = ", identifier);
		it = m_local_references.emplace(identifier, nullptr).first;
	}
	trivialize_reference(reference_opt);
	it->second = reference_opt.release();
}
void Scope::clear_local_references() noexcept {
	m_local_references.clear();
}

Xptr<Reference> Scope::get_reference_recursive_opt(const std::string &identifier) const noexcept {
	for(auto scope = this; scope; scope = scope->get_parent_opt().get()){
		auto reference = scope->get_local_reference_opt(identifier);
		if(reference){
			return std::move(reference);
		}
	}
	return nullptr;
}

}
