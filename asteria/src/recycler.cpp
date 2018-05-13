// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "recycler.hpp"
#include "variable.hpp"
#include "utilities.hpp"
#include <algorithm>

namespace Asteria {

Recycler::~Recycler(){
	do_purge_variables();
}

void Recycler::do_purge_variables() noexcept {
	while(m_weak_variables.empty() == false){
		purge_variable(m_weak_variables.back().lock());
		m_weak_variables.pop_back();
	}
}

void Recycler::adopt_variable(Sparg<Variable> variable_opt){
	if(!variable_opt){
		return;
	}
	defragment();
	m_weak_variables.emplace_back(variable_opt);
}
void Recycler::defragment(bool aggressive) noexcept {
	const auto threshold_old = m_defragmentation_threshold;
	if(!aggressive && (m_weak_variables.size() < threshold_old)){
		return;
	}
	ASTERIA_DEBUG_LOG("Performing automatic defragmentation: size = ", m_weak_variables.size());
	const auto erased_begin = std::remove_if(m_weak_variables.begin(), m_weak_variables.end(), [](Wparg<Variable> weak_ref){ return weak_ref.expired(); });
	ASTERIA_DEBUG_LOG("Removing dead objects: count_removed = ", std::distance(erased_begin, m_weak_variables.end()));
	m_weak_variables.erase(erased_begin, m_weak_variables.end());
	const auto threshold_new = std::max(threshold_old, m_weak_variables.size() + defragmentation_threshold_increment);
	ASTERIA_DEBUG_LOG("Setting new defragmentation threshold: threshold_old = ", threshold_old, ", threshold_new = ", threshold_new);
	m_defragmentation_threshold = threshold_new;
}

}
