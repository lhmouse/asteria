// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "initializer.hpp"
#include "stored_value.hpp"
#include "stored_reference.hpp"
#include "reference.hpp"
#include "expression.hpp"
#include "utilities.hpp"

namespace Asteria {

Initializer::Initializer(Initializer &&) noexcept = default;
Initializer & Initializer::operator=(Initializer &&) noexcept = default;
Initializer::~Initializer() = default;

Initializer::Type get_initializer_type(Spparam<const Initializer> initializer_opt) noexcept {
	return initializer_opt ? initializer_opt->get_type() : Initializer::type_null;
}

void bind_initializer(Xptr<Initializer> &bound_result_out, Spparam<const Initializer> initializer_opt, Spparam<const Scope> scope){
	const auto type = get_initializer_type(initializer_opt);
	switch(type){
	case Initializer::type_null:
		return bound_result_out.reset();

	case Initializer::type_assignment_init: {
		const auto &params = initializer_opt->get<Initializer::S_assignment_init>();
		Xptr<Expression> bound_expr;
		bind_expression(bound_expr, params.expression, scope);
		Initializer::S_assignment_init init_a = { std::move(bound_expr) };
		return bound_result_out.emplace(std::move(init_a)); }

	case Initializer::type_bracketed_init_list: {
		const auto &params = initializer_opt->get<Initializer::S_bracketed_init_list>();
		Xptr_vector<Initializer> bound_elems;
		bound_elems.reserve(params.elements.size());
		for(const auto &elem : params.elements){
			bind_initializer(bound_result_out, elem, scope);
			bound_elems.emplace_back(std::move(bound_result_out));
		}
		Initializer::S_bracketed_init_list init_bracketed = { std::move(bound_elems) };
		return bound_result_out.emplace(std::move(init_bracketed)); }

	case Initializer::type_braced_init_list: {
		const auto &params = initializer_opt->get<Initializer::S_braced_init_list>();
		Xptr_string_map<Initializer> bound_pairs;
		bound_pairs.reserve(params.key_values.size());
		for(const auto &pair : params.key_values){
			bind_initializer(bound_result_out, pair.second, scope);
			bound_pairs.emplace(pair.first, std::move(bound_result_out));
		}
		Initializer::S_braced_init_list init_braced = { std::move(bound_pairs) };
		return bound_result_out.emplace(std::move(init_braced)); }

	default:
		ASTERIA_DEBUG_LOG("Unknown initializer type enumeration: type = ", type);
		std::terminate();
	}
}
void evaluate_initializer(Xptr<Reference> &reference_out, Spparam<Recycler> recycler, Spparam<const Initializer> initializer_opt, Spparam<const Scope> scope){
	const auto type = get_initializer_type(initializer_opt);
	switch(type){
	case Initializer::type_null:
		return set_reference(reference_out, nullptr);

	case Initializer::type_assignment_init: {
		const auto &params = initializer_opt->get<Initializer::S_assignment_init>();
		return evaluate_expression(reference_out, recycler, params.expression, scope); }

	case Initializer::type_bracketed_init_list: {
		const auto &params = initializer_opt->get<Initializer::S_bracketed_init_list>();
		Xptr<Variable> var;
		D_array array;
		array.reserve(params.elements.size());
		for(const auto &elem : params.elements){
			evaluate_initializer(reference_out, recycler, elem, scope);
			extract_variable_from_reference(var, recycler, std::move(reference_out));
			array.emplace_back(std::move(var));
		}
		set_variable(var, recycler, std::move(array));
		Reference::S_temporary_value ref_t = { std::move(var) };
		return set_reference(reference_out, std::move(ref_t)); }

	case Initializer::type_braced_init_list: {
		const auto &params = initializer_opt->get<Initializer::S_braced_init_list>();
		Xptr<Variable> var;
		D_object object;
		object.reserve(params.key_values.size());
		for(const auto &pair : params.key_values){
			evaluate_initializer(reference_out, recycler, pair.second, scope);
			extract_variable_from_reference(var, recycler, std::move(reference_out));
			object.emplace(pair.first, std::move(var));
		}
		set_variable(var, recycler, std::move(object));
		Reference::S_temporary_value ref_t = { std::move(var) };
		return set_reference(reference_out, std::move(ref_t)); }

	default:
		ASTERIA_DEBUG_LOG("Unknown initializer type enumeration: type = ", type);
		std::terminate();
	}
}

}
