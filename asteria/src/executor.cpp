// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "executor.hpp"
#include "recycler.hpp"
#include "scope.hpp"
#include "block.hpp"
#include "stored_value.hpp"
#include "stored_reference.hpp"
#include "function_base.hpp"
#include "simple_function.hpp"

namespace Asteria {

Executor::~Executor() = default;

const Sptr<Recycler> & Executor::do_get_recycler(){
	if(m_recycler_opt == nullptr){
		m_recycler_opt = std::make_shared<Recycler>();
	}
	return m_recycler_opt;
}
const Sptr<Scope> & Executor::do_get_root_scope(){
	if(m_root_scope_opt == nullptr){
		m_root_scope_opt = std::make_shared<Scope>(Scope::purpose_file, nullptr);
	}
	return m_root_scope_opt;
}

void Executor::reset() noexcept {
	m_recycler_opt.reset();
	m_root_scope_opt.reset();
	m_code_opt.reset();
}

void Executor::set_root_variable(const String &identifier, Stored_value &&value, bool constant){
	// Create a reference.
	Xptr<Variable> var;
	set_variable(var, do_get_recycler(), std::move(value));
	Xptr<Reference> ref;
	Reference::S_temporary_value ref_t = { std::move(var) };
	set_reference(ref, std::move(ref_t));
	materialize_reference(ref, do_get_recycler(), constant);
	// Insert it into the root scope.
	const auto wref = do_get_root_scope()->drill_for_local_reference(identifier);
	return move_reference(wref, std::move(ref));
}
void Executor::set_root_constant(const String &identifier, Stored_value &&value){
	return set_root_variable(identifier, std::move(value), true);
}
void Executor::set_root_function(const String &identifier, Sptr<const Function_base> &&func){
	return set_root_constant(identifier, std::move(func));
}
void Executor::set_root_function(const String &identifier, String description, Function_base_prototype *target){
	return set_root_function(identifier, std::make_shared<Simple_function>(std::move(description), target));
}

}
