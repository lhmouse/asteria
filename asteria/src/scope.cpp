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

Sptr<const Reference> Scope::get_local_reference_opt(const D_string &identifier) const noexcept {
	auto it = m_local_references.find(identifier);
	if(it == m_local_references.end()){
		return nullptr;
	}
	return it->second;
}
std::reference_wrapper<Xptr<Reference>> Scope::drill_for_local_reference(const D_string &identifier){
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
	class Argument_getter : public Function_base {
	private:
		D_string m_self_identifier;
		D_string m_source_location;
		Xptr_vector<Reference> m_arguments_opt;

	public:
		Argument_getter(const D_string &self_identifier, const D_string &source_location, Xptr_vector<Reference> &&arguments_opt)
			: m_self_identifier(self_identifier), m_source_location(source_location), m_arguments_opt(std::move(arguments_opt))
		{ }

	public:
		D_string describe() const override {
			return ASTERIA_FORMAT_STRING("variadic argument getter @ '", m_source_location, "'");
		}
		void invoke(Xptr<Reference> &result_out, Spparam<Recycler> recycler, Xptr<Reference> &&/*this_opt*/, Xptr_vector<Reference> &&arguments_opt) const override {
			switch(arguments_opt.size()){
			case 0: {
				// Return the number of arguments.
				Xptr<Value> value;
				set_value(value, recycler, D_integer(static_cast<std::ptrdiff_t>(m_arguments_opt.size())));
				Reference::S_temporary_value ref_t = { std::move(value) };
				return set_reference(result_out, std::move(ref_t)); }

			case 1: {
				// Return the argument at the index specified.
				const auto index_var = read_reference_opt(arguments_opt.at(0));
				if(get_value_type(index_var) != Value::type_integer){
					ASTERIA_THROW_RUNTIME_ERROR("The argument passed to `", m_self_identifier, "` must be an `integer`.");
				}
				// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
				const auto index = index_var->get<D_integer>();
				auto normalized_index = (index >= 0) ? index : D_integer(Unsigned_integer(index) + m_arguments_opt.size());
				if(normalized_index < 0){
					ASTERIA_DEBUG_LOG("Argument subscript falls before the front: index = ", index, ", size = ", m_arguments_opt.size());
					return set_reference(result_out, nullptr);
				} else if(normalized_index >= D_integer(m_arguments_opt.size())){
					ASTERIA_DEBUG_LOG("Argument subscript falls after the back: index = ", index, ", size = ", m_arguments_opt.size());
					return set_reference(result_out, nullptr);
				}
				const auto &arg = m_arguments_opt.at(static_cast<std::size_t>(normalized_index));
				return copy_reference(result_out, arg); }

			default:
				ASTERIA_THROW_RUNTIME_ERROR("`", m_self_identifier, "` accepts no more than one argument.");
			}
		}
	};

	void do_set_argument(Spparam<Scope> scope, const D_string &identifier, Xptr<Reference> &&arg_opt){
		if(identifier.empty()){
			return;
		}
		const auto wref = scope->drill_for_local_reference(identifier);
		move_reference(wref, std::move(arg_opt));
	}
	void do_set_argument(Spparam<Scope> scope, Spparam<const Parameter> param_opt, Xptr<Reference> &&arg_opt){
		if(param_opt == nullptr){
			return;
		}
		do_set_argument(scope, param_opt->get_identifier(), std::move(arg_opt));
	}

	void do_shift_argument(Spparam<Scope> scope, Xptr_vector<Reference> &arguments_inout_opt, Spparam<const Parameter> param_opt){
		Xptr<Reference> arg_opt;
		if(arguments_inout_opt.empty() == false){
			arg_opt = std::move(arguments_inout_opt.front());
			arguments_inout_opt.erase(arguments_inout_opt.begin());
		}
		do_set_argument(scope, param_opt, std::move(arg_opt));
	}

	void do_create_argument_getter(Spparam<Scope> scope, const D_string &identifier, const D_string &description, Xptr_vector<Reference> &&arguments_opt){
		auto value = std::make_shared<Value>(D_function(std::make_shared<Argument_getter>(identifier, description, std::move(arguments_opt))));
		Xptr<Reference> arg;
		Reference::S_constant ref_k = { std::move(value) };
		set_reference(arg, std::move(ref_k));
		do_set_argument(scope, identifier, std::move(arg));
	}
	void do_create_source_reference(Spparam<Scope> scope, const D_string &identifier, const D_string &description){
		auto value = std::make_shared<Value>(D_string(description));
		Xptr<Reference> arg;
		Reference::S_constant ref_k = { std::move(value) };
		set_reference(arg, std::move(ref_k));
		do_set_argument(scope, identifier, std::move(arg));
	}
}

void prepare_function_scope(Spparam<Scope> scope, Spparam<Recycler> recycler, const D_string &source_location, const Sptr_vector<const Parameter> &parameters_opt, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt){
	// Materialize everything, as function parameters should be modifiable.
	materialize_reference(this_opt, recycler, true);
	std::for_each(arguments_opt.begin(), arguments_opt.end(), [&](Xptr<Reference> &arg_opt){ materialize_reference(arg_opt, recycler, true); });
	// Move arguments into the local scope.
	do_set_argument(scope, D_string::shallow("this"), std::move(this_opt));
	std::for_each(parameters_opt.begin(), parameters_opt.end(), [&](Spparam<const Parameter> param_opt){ do_shift_argument(scope, arguments_opt, param_opt); });
	// Create pre-defined variables.
	do_create_source_reference(scope, D_string::shallow("__source"), source_location);
	do_create_argument_getter(scope, D_string::shallow("__va_arg"), source_location, std::move(arguments_opt));
}
void prepare_function_scope_lexical(Spparam<Scope> scope, const D_string &source_location, const Sptr_vector<const Parameter> &parameters_opt){
	// Create null arguments in the local scope.
	do_set_argument(scope, D_string::shallow("this"), nullptr);
	std::for_each(parameters_opt.begin(), parameters_opt.end(), [&](Spparam<const Parameter> param_opt){ do_set_argument(scope, param_opt, nullptr); });
	// Create pre-defined variables.
	do_create_source_reference(scope, D_string::shallow("__source"), source_location);
	do_set_argument(scope, D_string::shallow("__va_arg"), nullptr);
}

}
