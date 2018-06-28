// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "recycler.hpp"
#include "value.hpp"
#include "utilities.hpp"
#include <algorithm>

namespace Asteria {

Recycler::~Recycler(){
	do_purge_values();
}

void Recycler::do_purge_values() noexcept {
	while(m_weak_values.empty() == false){
		wipe_out_value(m_weak_values.back().lock());
		m_weak_values.pop_back();
	}
}

void Recycler::adopt_value(Spr<Value> value_opt){
	if(!value_opt){
		return;
	}
	defragment();
	m_weak_values.emplace_back(value_opt);
}
void Recycler::defragment(bool aggressive) noexcept {
	const auto threshold_old = m_defragmentation_threshold;
	if(!aggressive && (m_weak_values.size() < threshold_old)){
		return;
	}
	ASTERIA_DEBUG_LOG("Performing automatic defragmentation: size = ", m_weak_values.size());
	const auto erased_begin = std::remove_if(m_weak_values.begin(), m_weak_values.end(), [](Wpr<Value> weak_ref){ return weak_ref.expired(); });
	ASTERIA_DEBUG_LOG("Removing dead objects: count_removed = ", std::distance(erased_begin, m_weak_values.end()));
	m_weak_values.erase(erased_begin, m_weak_values.end());
	const auto threshold_new = std::max(threshold_old, m_weak_values.size() + defragmentation_threshold_increment);
	ASTERIA_DEBUG_LOG("Setting new defragmentation threshold: threshold_old = ", threshold_old, ", threshold_new = ", threshold_new);
	m_defragmentation_threshold = threshold_new;
}

}
