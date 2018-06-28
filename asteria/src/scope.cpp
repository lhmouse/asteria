// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "scope.hpp"
#include "value.hpp"
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
	Vp<Reference> unused_result;
	while(m_deferred_callbacks.empty() == false){
		m_deferred_callbacks.back()->invoke(unused_result, nullptr, nullptr, { });
		m_deferred_callbacks.pop_back();
	}
} catch(std::exception &e){
	ASTERIA_DEBUG_LOG("Ignoring `std::exception` thrown from deferred callbacks: ", e.what());
}

Sp<const Reference> Scope::get_named_reference_opt(const D_string &id) const noexcept {
	auto it = m_named_references.find(id);
	if(it == m_named_references.end()){
		return nullptr;
	}
	return it->second;
}
std::reference_wrapper<Vp<Reference>> Scope::drill_for_named_reference(const D_string &id){
	if(id.empty()){
		ASTERIA_THROW_RUNTIME_ERROR("Identifiers of variables must not be empty.");
	}
	auto it = m_named_references.find(id);
	if(it == m_named_references.end()){
		ASTERIA_DEBUG_LOG("Creating named reference: id = ", id);
		it = m_named_references.emplace(id, nullptr).first;
	}
	return it->second;
}

void Scope::defer_callback(Sp<const Function_base> &&callback){
	m_deferred_callbacks.emplace_back(std::move(callback));
}

namespace {
	class Argument_getter : public Function_base {
	private:
		D_string m_self_id;
		D_string m_source_location;
		Vp_vector<Reference> m_arguments_opt;

	public:
		Argument_getter(const D_string &self_id, const D_string &location, Vp_vector<Reference> &&arguments_opt)
			: m_self_id(self_id), m_source_location(location), m_arguments_opt(std::move(arguments_opt))
		{ }

	public:
		D_string describe() const override {
			return ASTERIA_FORMAT_STRING("variadic argument getter @ '", m_source_location, "'");
		}
		void invoke(Vp<Reference> &result_out, Spr<Recycler> recycler_inout, Vp<Reference> &&/*this_opt*/, Vp_vector<Reference> &&arguments_opt) const override {
			switch(arguments_opt.size()){
			case 0: {
				// Return the number of arguments.
				Vp<Value> value;
				set_value(value, recycler_inout, D_integer(static_cast<std::ptrdiff_t>(m_arguments_opt.size())));
				Reference::S_temporary_value ref_t = { std::move(value) };
				return set_reference(result_out, std::move(ref_t)); }

			case 1: {
				// Return the argument at the index specified.
				const auto index_var = read_reference_opt(arguments_opt.at(0));
				if(get_value_type(index_var) != Value::type_integer){
					ASTERIA_THROW_RUNTIME_ERROR("The argument passed to `", m_self_id, "` must be an `integer`.");
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
				ASTERIA_THROW_RUNTIME_ERROR("`", m_self_id, "` accepts no more than one argument.");
			}
		}
	};

	void do_set_argument(Spr<Scope> scope, const D_string &id, Vp<Reference> &&arg_opt){
		if(id.empty()){
			return;
		}
		const auto wref = scope->drill_for_named_reference(id);
		move_reference(wref, std::move(arg_opt));
	}
	void do_set_argument(Spr<Scope> scope, Spr<const Parameter> param_opt, Vp<Reference> &&arg_opt){
		if(param_opt == nullptr){
			return;
		}
		do_set_argument(scope, param_opt->get_id(), std::move(arg_opt));
	}

	void do_shift_argument(Spr<Scope> scope, Vp_vector<Reference> &arguments_inout_opt, Spr<const Parameter> param_opt){
		Vp<Reference> arg_opt;
		if(arguments_inout_opt.empty() == false){
			arg_opt = std::move(arguments_inout_opt.front());
			arguments_inout_opt.erase(arguments_inout_opt.begin());
		}
		do_set_argument(scope, param_opt, std::move(arg_opt));
	}

	void do_create_argument_getter(Spr<Scope> scope, const D_string &id, const D_string &description, Vp_vector<Reference> &&arguments_opt){
		auto value = std::make_shared<Value>(D_function(std::make_shared<Argument_getter>(id, description, std::move(arguments_opt))));
		Vp<Reference> arg;
		Reference::S_constant ref_k = { std::move(value) };
		set_reference(arg, std::move(ref_k));
		do_set_argument(scope, id, std::move(arg));
	}
	void do_create_source_reference(Spr<Scope> scope, const D_string &id, const D_string &description){
		auto value = std::make_shared<Value>(D_string(description));
		Vp<Reference> arg;
		Reference::S_constant ref_k = { std::move(value) };
		set_reference(arg, std::move(ref_k));
		do_set_argument(scope, id, std::move(arg));
	}
}

void prepare_function_scope(Spr<Scope> scope, Spr<Recycler> recycler_inout, const D_string &location, const Sp_vector<const Parameter> &params_opt, Vp<Reference> &&this_opt, Vp_vector<Reference> &&arguments_opt){
	// Materialize everything, as function parameters should be modifiable.
	materialize_reference(this_opt, recycler_inout, true);
	std::for_each(arguments_opt.begin(), arguments_opt.end(), [&](Vp<Reference> &arg_opt){ materialize_reference(arg_opt, recycler_inout, true); });
	// Move arguments into the scope.
	do_set_argument(scope, D_string::shallow("this"), std::move(this_opt));
	std::for_each(params_opt.begin(), params_opt.end(), [&](Spr<const Parameter> param_opt){ do_shift_argument(scope, arguments_opt, param_opt); });
	// Create pre-defined variables.
	do_create_source_reference(scope, D_string::shallow("__source"), location);
	do_create_argument_getter(scope, D_string::shallow("__va_arg"), location, std::move(arguments_opt));
}
void prepare_function_scope_lexical(Spr<Scope> scope, const D_string &location, const Sp_vector<const Parameter> &params_opt){
	// Create null arguments in the scope.
	do_set_argument(scope, D_string::shallow("this"), nullptr);
	std::for_each(params_opt.begin(), params_opt.end(), [&](Spr<const Parameter> param_opt){ do_set_argument(scope, param_opt, nullptr); });
	// Create pre-defined variables.
	do_create_source_reference(scope, D_string::shallow("__source"), location);
	do_set_argument(scope, D_string::shallow("__va_arg"), nullptr);
}

}
