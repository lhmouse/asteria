// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "scope.hpp"
#include "stored_value.hpp"
#include "stored_reference.hpp"
#include "function_base.hpp"
#include "parameter.hpp"
#include "utilities.hpp"
#include <algorithm>

namespace Asteria {

Scope::~Scope(){
	do_dispose_deferred_callbacks();
}

void Scope::do_dispose_deferred_callbacks() noexcept
try {
	Xptr<Reference> unused_result;
	while(m_deferred_callbacks.empty() == false){
		m_deferred_callbacks.back()->invoke(unused_result, nullptr, nullptr, { });
		m_deferred_callbacks.pop_back();
	}
} catch(std::exception &e){
	ASTERIA_DEBUG_LOG("Ignoring `std::exception` thrown from deferred callbacks: ", e.what());
}

Sptr<const Reference> Scope::get_local_reference_opt(const String &identifier) const noexcept {
	auto it = m_local_references.find(identifier);
	if(it == m_local_references.end()){
		return nullptr;
	}
	return it->second;
}
std::reference_wrapper<Xptr<Reference>> Scope::drill_for_local_reference(const String &identifier){
	if(identifier.empty()){
		ASTERIA_THROW_RUNTIME_ERROR("Identifiers of local variables or constants must not be empty.");
	}
	auto it = m_local_references.find(identifier);
	if(it == m_local_references.end()){
		ASTERIA_DEBUG_LOG("Creating local reference: identifier = ", identifier);
		it = m_local_references.emplace(identifier, nullptr).first;
	}
	return it->second;
}

void Scope::defer_callback(Sptr<const Function_base> &&callback){
	m_deferred_callbacks.emplace_back(std::move(callback));
}

namespace {
	class Variadic_argument_getter : public Function_base {
	private:
		Xptr_vector<Reference> m_arguments_opt;

	public:
		Variadic_argument_getter(String description, Xptr_vector<Reference> arguments_opt)
			: Function_base(std::move(description))
			, m_arguments_opt(std::move(arguments_opt))
		{ }

	public:
		void invoke(Xptr<Reference> &result_out, Sparg<Recycler> recycler, Xptr<Reference> &&/*this_opt*/, Xptr_vector<Reference> &&arguments_opt) const override {
			switch(arguments_opt.size()){
			case 0: {
				// Return the number of arguments.
				Xptr<Variable> var;
				set_variable(var, recycler, D_integer(static_cast<std::ptrdiff_t>(m_arguments_opt.size())));
				Reference::S_temporary_value ref_t = { std::move(var) };
				return set_reference(result_out, std::move(ref_t)); }

			case 1: {
				// Return the argument at the index specified.
				const auto index_var = read_reference_opt(arguments_opt.at(0));
				if(get_variable_type(index_var) != Variable::type_integer){
					ASTERIA_THROW_RUNTIME_ERROR("The argument passed to `__va_arg` must be an `integer`.");
				}
				// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
				const auto index = index_var->get<D_integer>();
				auto normalized_index = (index >= 0) ? index : static_cast<std::int64_t>(static_cast<std::uint64_t>(index) + m_arguments_opt.size());
				if(normalized_index < 0){
					ASTERIA_DEBUG_LOG("Argument subscript falls before the front: index = ", index, ", size = ", m_arguments_opt.size());
					return set_reference(result_out, nullptr);
				} else if(normalized_index >= static_cast<std::int64_t>(m_arguments_opt.size())){
					ASTERIA_DEBUG_LOG("Argument subscript falls after the back: index = ", index, ", size = ", m_arguments_opt.size());
					return set_reference(result_out, nullptr);
				}
				const auto &arg = m_arguments_opt.at(static_cast<std::size_t>(normalized_index));
				return copy_reference(result_out, arg); }

			default:
				ASTERIA_THROW_RUNTIME_ERROR("`__va_arg` accepts no more than one argument.");
			}
		}
	};
}

void prepare_function_scope(Sparg<Scope> scope, Sparg<Recycler> recycler, const String &description, Sparg<const T_vector<Parameter>> parameters_opt, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt){
	// Materialize everything first.
	materialize_reference(this_opt, recycler, true);
	std::for_each(arguments_opt.begin(), arguments_opt.end(), [&](Xptr<Reference> &arg_opt){ materialize_reference(arg_opt, recycler, true); });

	// Set the `this` reference.
	auto wref = scope->drill_for_local_reference(String::shallow("this"));
	move_reference(wref, std::move(this_opt));

	// Move arguments into local scope. Unlike the `this` reference, a named argument has to exist even when it is not provided.
	if(parameters_opt){
		for(const auto &param : *parameters_opt){
			Xptr<Reference> arg;
			if(!arguments_opt.empty()){
				move_reference(arg, std::move(arguments_opt.front()));
				arguments_opt.erase(arguments_opt.begin());
			}
			const auto &identifier = param.get_identifier();
			if(identifier.empty()){
				continue;
			}
			wref = scope->drill_for_local_reference(identifier);
			move_reference(wref, std::move(arg));
		}
	}

	// Set argument getter for variadic functions.
	rocket::insertable_ostream desc_os;
	desc_os <<"variadic argument getter for " <<description;
	Xptr<Variable> var;
	auto va_arg_func = std::make_shared<Variadic_argument_getter>(desc_os.extract_string(), std::move(arguments_opt));
	set_variable(var, recycler, D_function(std::move(va_arg_func)));
	wref = scope->drill_for_local_reference(String::shallow("__va_arg"));
	Reference::S_constant ref_kv = { std::move(var) };
	set_reference(wref, std::move(ref_kv));

	// Set predefined variables.
	set_variable(var, recycler, D_string(description));
	wref = scope->drill_for_local_reference(String::shallow("__func"));
	Reference::S_constant ref_kf = { std::move(var) };
	set_reference(wref, std::move(ref_kf));
}
void prepare_function_scope_lexical(Sparg<Scope> scope, Sparg<const T_vector<Parameter>> parameters_opt){
	// Set the `this` reference.
	auto wref = scope->drill_for_local_reference(String::shallow("this"));
	set_reference(wref, nullptr);

	// Move arguments into local scope. Unlike the `this` reference, a named argument has to exist even when it is not provided.
	if(parameters_opt){
		for(const auto &param : *parameters_opt){
			const auto &identifier = param.get_identifier();
			if(identifier.empty()){
				continue;
			}
			wref = scope->drill_for_local_reference(identifier);
			set_reference(wref, nullptr);
		}
	}

	// Set argument getter for variadic functions.
	wref = scope->drill_for_local_reference(String::shallow("__va_arg"));
	set_reference(wref, nullptr);

	// Set predefined variables.
	wref = scope->drill_for_local_reference(String::shallow("__func"));
	set_reference(wref, nullptr);
}

}
