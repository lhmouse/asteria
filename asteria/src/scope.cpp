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

Sp<const Reference> Scope::get_named_reference_opt(Cow_string_ref id) const noexcept {
	auto it = m_named_references.find(id);
	if(it == m_named_references.end()){
		return nullptr;
	}
	return it->second;
}
std::reference_wrapper<Vp<Reference>> Scope::drill_for_named_reference(Cow_string_ref id){
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
		D_string m_source;
		Vector<Vp<Reference>> m_args;

	public:
		Argument_getter(const D_string &self_id, const D_string &location, Vector<Vp<Reference>> &&args)
			: m_self_id(self_id), m_source(location), m_args(std::move(args))
		{ }

	public:
		D_string describe() const override {
			return ASTERIA_FORMAT_STRING("variadic argument getter @ '", m_source, "'");
		}
		void invoke(Vp<Reference> &result_out, Sp_ref<Recycler> recycler_out, Vp<Reference> &&/*this_opt*/, Vector<Vp<Reference>> &&args) const override {
			switch(args.size()){
			case 0: {
				// Return the number of args.
				Vp<Value> value;
				set_value(value, recycler_out, D_integer(static_cast<std::ptrdiff_t>(m_args.size())));
				Reference::S_temporary_value ref_t = { std::move(value) };
				return set_reference(result_out, std::move(ref_t)); }

			case 1: {
				// Return the argument at the index specified.
				const auto index_var = read_reference_opt(args.at(0));
				if(get_value_type(index_var) != Value::type_integer){
					ASTERIA_THROW_RUNTIME_ERROR("The argument passed to `", m_self_id, "` must be an `integer`.");
				}
				// If a negative index is provided, wrap it around the array once to get the actual subscript. Note that the result may still be negative.
				const auto index = index_var->get<D_integer>();
				auto normalized_index = (index >= 0) ? index : D_integer(Unsigned_integer(index) + m_args.size());
				if(normalized_index < 0){
					ASTERIA_DEBUG_LOG("Argument subscript falls before the front: index = ", index, ", size = ", m_args.size());
					return set_reference(result_out, nullptr);
				} else if(normalized_index >= D_integer(m_args.size())){
					ASTERIA_DEBUG_LOG("Argument subscript falls after the back: index = ", index, ", size = ", m_args.size());
					return set_reference(result_out, nullptr);
				}
				const auto &arg = m_args.at(static_cast<std::size_t>(normalized_index));
				return copy_reference(result_out, arg); }

			default:
				ASTERIA_THROW_RUNTIME_ERROR("`", m_self_id, "` accepts no more than one argument.");
			}
		}
	};

	void do_set_argument(Sp_ref<Scope> scope, Cow_string_ref id, Vp<Reference> &&arg_opt){
		if(id.empty()){
			return;
		}
		const auto wref = scope->drill_for_named_reference(id);
		move_reference(wref, std::move(arg_opt));
	}
	void do_set_argument(Sp_ref<Scope> scope, const Parameter &param, Vp<Reference> &&arg_opt){
		const auto &id = param.get_id();
		if(id.empty()){
			return;
		}
		do_set_argument(scope, id, std::move(arg_opt));
	}

	void do_shift_argument(Sp_ref<Scope> scope, Vector<Vp<Reference>> &args_inout_opt, const Parameter &param){
		Vp<Reference> arg_opt;
		if(args_inout_opt.empty() == false){
			arg_opt = std::move(args_inout_opt.front());
			args_inout_opt.erase(args_inout_opt.begin());
		}
		do_set_argument(scope, param, std::move(arg_opt));
	}

	void do_create_argument_getter(Sp_ref<Scope> scope, Cow_string_ref id, const D_string &description, Vector<Vp<Reference>> &&args){
		auto value = std::make_shared<Value>(D_function(std::make_shared<Argument_getter>(id, description, std::move(args))));
		Vp<Reference> arg;
		Reference::S_constant ref_k = { std::move(value) };
		set_reference(arg, std::move(ref_k));
		do_set_argument(scope, id, std::move(arg));
	}
	void do_create_source_reference(Sp_ref<Scope> scope, Cow_string_ref id, const D_string &description){
		auto value = std::make_shared<Value>(D_string(description));
		Vp<Reference> arg;
		Reference::S_constant ref_k = { std::move(value) };
		set_reference(arg, std::move(ref_k));
		do_set_argument(scope, id, std::move(arg));
	}
}

void prepare_function_scope(Sp_ref<Scope> scope, Sp_ref<Recycler> recycler_out, Cow_string_ref source, const Vector<Parameter> &params, Vp<Reference> &&this_opt, Vector<Vp<Reference>> &&args){
	// Materialize everything, as function parameters should be modifiable.
	materialize_reference(this_opt, recycler_out, true);
	std::for_each(args.begin(), args.end(), [&](Vp<Reference> &arg_opt){ materialize_reference(arg_opt, recycler_out, true); });
	// Move arguments into the scope.
	do_set_argument(scope, Cow_string::shallow("this"), std::move(this_opt));
	std::for_each(params.begin(), params.end(), [&](const Parameter &param){ do_shift_argument(scope, args, param); });
	// Create pre-defined variables.
	do_create_source_reference(scope, Cow_string::shallow("__source"), source);
	do_create_argument_getter(scope, Cow_string::shallow("__va_arg"), source, std::move(args));
}
void prepare_function_scope_lexical(Sp_ref<Scope> scope, Cow_string_ref source, const Vector<Parameter> &params){
	// Create null parameters in the scope.
	do_set_argument(scope, Cow_string::shallow("this"), nullptr);
	std::for_each(params.begin(), params.end(), [&](const Parameter &param){ do_set_argument(scope, param, nullptr); });
	// Create pre-defined variables.
	do_create_source_reference(scope, Cow_string::shallow("__source"), source);
	do_set_argument(scope, Cow_string::shallow("__va_arg"), nullptr);
}

}
