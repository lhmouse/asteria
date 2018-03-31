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

void Recycler::set_variable(Value_ptr<Variable> &variable_out_opt, Stored_value &&value_opt){
	if(!value_opt){
		variable_out_opt = nullptr;
	} else if(!variable_out_opt){
		auto xptr = create_shared<Variable>(std::move(value_opt.get()));
		defragment_automatic();
		m_weak_variables.emplace_back(xptr);
		variable_out_opt = Value_ptr<Variable>(std::move(xptr));
	} else {
		*variable_out_opt = Variable(std::move(value_opt.get()));
	}
}
void Recycler::copy_variable(Value_ptr<Variable> &variable_out_opt, Spref<const Variable> source_opt){
	if(!source_opt){
		set_variable(variable_out_opt, nullptr);
	} else {
		set_variable(variable_out_opt, *source_opt);
	}
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
