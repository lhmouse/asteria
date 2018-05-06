// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "scope.hpp"
#include "stored_value.hpp"
#include "stored_reference.hpp"
#include "function_base.hpp"
#include "parameter.hpp"
#include "utilities.hpp"

namespace Asteria {

Scope::~Scope(){
	for(auto it = m_deferred_callbacks.rbegin(); it != m_deferred_callbacks.rend(); ++it){
		const auto &func = *it;
		try {
			Xptr<Reference> result;
			func->invoke(result, nullptr, nullptr, { });
		} catch(std::exception &e){
			ASTERIA_DEBUG_LOG("`std::exception` thrown from a deferred callback has been ignored: ", e.what());
		}
	}
}

Sptr<const Reference> Scope::get_local_reference_opt(const std::string &identifier) const noexcept {
	auto it = m_local_references.find(identifier);
	if(it == m_local_references.end()){
		return nullptr;
	}
	return it->second;
}
std::reference_wrapper<Xptr<Reference>> Scope::drill_for_local_reference(const std::string &identifier){
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
	const std::string g_id_this    = "this";
	const std::string g_id_va_arg  = "va_arg";

	class Variadic_argument_getter : public Function_base {
	private:
		Xptr_vector<Reference> m_arguments_opt;

	public:
		explicit Variadic_argument_getter(Xptr_vector<Reference> arguments_opt)
			: m_arguments_opt(std::move(arguments_opt))
		{ }

	public:
		const char * describe() const noexcept override {
			return "variadic argument getter";
		}
		void invoke(Xptr<Reference> &result_out, Spcref<Recycler> recycler, Xptr<Reference> &&/*this_opt*/, Xptr_vector<Reference> &&arguments_opt) const override {
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
					ASTERIA_THROW_RUNTIME_ERROR("The argument passed to `", g_id_va_arg, "` must be an `integer`.");
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
				ASTERIA_THROW_RUNTIME_ERROR("`", g_id_va_arg, "` accepts no more than one argument.");
			}
		}
	};
}

void prepare_function_scope(Spcref<Scope> scope, Spcref<Recycler> recycler, Spcref<const Parameter_vector> parameters_opt, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt){
	// Materialize everything first.
	materialize_reference(this_opt, recycler, true);
	std::for_each(arguments_opt.begin(), arguments_opt.end(), [&](Xptr<Reference> &arg_opt){ materialize_reference(arg_opt, recycler, true); });
	// Set the `this` reference.
	const auto this_wref = scope->drill_for_local_reference(g_id_this);
	move_reference(this_wref, std::move(this_opt));
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
			const auto param_wref = scope->drill_for_local_reference(identifier);
			move_reference(param_wref, std::move(arg));
		}
	}
	// Set argument getter for variadic functions.
	Xptr<Variable> va_arg_var;
	auto va_arg_func = std::make_shared<Variadic_argument_getter>(std::move(arguments_opt));
	set_variable(va_arg_var, recycler, D_function(std::move(va_arg_func)));
	const auto va_arg_wref = scope->drill_for_local_reference(g_id_va_arg);
	Reference::S_constant ref_k = { std::move(va_arg_var) };
	set_reference(va_arg_wref, std::move(ref_k));
}
void prepare_function_scope_lexical(Spcref<Scope> scope, Spcref<const Parameter_vector> parameters_opt){
	// Set the `this` reference.
	const auto this_wref = scope->drill_for_local_reference(g_id_this);
	set_reference(this_wref, nullptr);
	// Move arguments into local scope. Unlike the `this` reference, a named argument has to exist even when it is not provided.
	if(parameters_opt){
		for(const auto &param : *parameters_opt){
			const auto &identifier = param.get_identifier();
			if(identifier.empty()){
				continue;
			}
			const auto param_wref = scope->drill_for_local_reference(identifier);
			set_reference(param_wref, nullptr);
		}
	}
	// Set argument getter for variadic functions.
	const auto va_arg_wref = scope->drill_for_local_reference(g_id_va_arg);
	set_reference(va_arg_wref, nullptr);
}

}
