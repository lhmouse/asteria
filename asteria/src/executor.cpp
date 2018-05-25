// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "executor.hpp"
#include "recycler.hpp"
#include "scope.hpp"
#include "block.hpp"
#include "stored_value.hpp"
#include "stored_reference.hpp"
#include "local_variable.hpp"
#include "function_base.hpp"
#include "slim_function.hpp"

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

std::shared_ptr<Local_variable> Executor::set_root_variable(const D_string &identifier, Stored_value &&value, bool constant){
	Xptr<Variable> var;
	set_variable(var, do_get_recycler(), std::move(value));
	auto local_var = std::make_shared<Local_variable>(std::move(var), constant);
	// Create a reference.
	Xptr<Reference> ref;
	Reference::S_local_variable ref_l = { local_var };
	set_reference(ref, std::move(ref_l));
	// Insert it into the root scope.
	const auto wref = do_get_root_scope()->drill_for_local_reference(identifier);
	move_reference(wref, std::move(ref));
	// Return a pointer to the original local variable, allowing subsequent access to it.
	return local_var;
}
std::shared_ptr<Local_variable> Executor::set_root_constant(const D_string &identifier, Stored_value &&value){
	return set_root_variable(identifier, std::move(value), true);
}
std::shared_ptr<Local_variable> Executor::set_root_function(const D_string &identifier, Sptr<const Function_base> &&func){
	return set_root_constant(identifier, std::move(func));
}
std::shared_ptr<Local_variable> Executor::set_root_slim_function(const D_string &identifier, D_string description, Function_prototype *target){
	return set_root_function(identifier, std::make_shared<Slim_function>(std::move(description), target));
}

}
