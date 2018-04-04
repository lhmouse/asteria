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

void Recycler::set_variable(Xptr<Variable> &variable_out_opt, Stored_value &&value_opt){
	if(value_opt.is_null()){
		variable_out_opt = nullptr;
	} else if(!variable_out_opt){
		auto sptr = std::make_shared<Variable>(std::move(value_opt.get()));
		defragment_automatic();
		m_weak_variables.emplace_back(sptr);
		variable_out_opt.reset(std::move(sptr));
	} else {
		*variable_out_opt = Variable(std::move(value_opt.get()));
	}
}
void Recycler::copy_variable_recursive(Xptr<Variable> &variable_out_opt, Spref<const Variable> source_opt){
	const auto type = get_variable_type(source_opt);
	switch(type){
	case Variable::type_null:
		variable_out_opt = nullptr;
		return;
	case Variable::type_boolean: {
		const auto &source = source_opt->get<D_boolean>();
		set_variable(variable_out_opt, source);
		return; }
	case Variable::type_integer: {
		const auto &source = source_opt->get<D_integer>();
		set_variable(variable_out_opt, source);
		return; }
	case Variable::type_double: {
		const auto &source = source_opt->get<D_double>();
		set_variable(variable_out_opt, source);
		return; }
	case Variable::type_string: {
		const auto &source = source_opt->get<D_string>();
		set_variable(variable_out_opt, source);
		return; }
	case Variable::type_opaque:
		ASTERIA_THROW_RUNTIME_ERROR("Variables having opaque types cannot be copied");
		/*return;*/
	case Variable::type_function: {
		const auto &source = source_opt->get<D_function>();
		set_variable(variable_out_opt, source);
		return; }
	case Variable::type_array: {
		const auto &source = source_opt->get<D_array>();
		D_array array;
		array.reserve(source.size());
		for(const auto &elem : source){
			copy_variable_recursive(variable_out_opt, elem);
			array.emplace_back(std::move(variable_out_opt));
		}
		set_variable(variable_out_opt, std::move(array));
		return; }
	case Variable::type_object: {
		const auto &source = source_opt->get<D_object>();
		D_object object;
		object.reserve(source.size());
		for(const auto &pair : source){
			copy_variable_recursive(variable_out_opt, pair.second);
			object.emplace(pair.first, std::move(variable_out_opt));
		}
		set_variable(variable_out_opt, std::move(object));
		return; }
	default:
		ASTERIA_DEBUG_LOG("Unknown type enumeration `", type, "`. This is probably a bug, please report.");
		std::terminate();
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
