// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "recycler.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "utilities.hpp"

namespace Asteria {

Recycler::~Recycler(){
	clear_variables();
}

Sptr<Variable> Recycler::set_variable(Xptr<Variable> &variable_out, Stored_value &&value_opt){
	Sptr<Variable> sptr_old, sptr_new;
	if(value_opt.has_value()){
		sptr_new = std::make_shared<Variable>(std::move(value_opt.get()));
		defragment_automatic();
		m_weak_variables.emplace_back(sptr_new);
	}
	sptr_old = variable_out.release();
	variable_out.reset(std::move(sptr_new));
	return sptr_old;
}
void Recycler::defragment_automatic() noexcept {
	const auto threshold_old = m_defragmentation_threshold;
	if(m_weak_variables.size() < threshold_old){
		return;
	}
	ASTERIA_DEBUG_LOG("Performing automatic garbage defragmentation: size = ", m_weak_variables.size());
	const auto erased_begin = std::remove_if(m_weak_variables.begin(), m_weak_variables.end(), [](const std::weak_ptr<Variable> &weak_ref){ return weak_ref.expired(); });
	ASTERIA_DEBUG_LOG("Removing dead objects: count_removed = ", std::distance(erased_begin, m_weak_variables.end()));
	m_weak_variables.erase(erased_begin, m_weak_variables.end());
	const auto threshold_new = std::max(threshold_old, m_weak_variables.size() + defragmentation_threshold_increment);
	ASTERIA_DEBUG_LOG("Setting new defragmentation threshold: threshold_old = ", threshold_old, ", threshold_new = ", threshold_new);
	m_defragmentation_threshold = threshold_new;
}
void Recycler::clear_variables() noexcept {
	for(auto &weak_rvar : m_weak_variables){
		dispose_variable_recursive(weak_rvar.lock());
	}
	m_weak_variables.clear();
}

}
