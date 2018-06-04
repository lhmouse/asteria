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

Initializer::Type get_initializer_type(Spr<const Initializer> initializer_opt) noexcept {
	return initializer_opt ? initializer_opt->get_type() : Initializer::type_null;
}

void bind_initializer(Vp<Initializer> &bound_result_out, Spr<const Initializer> initializer_opt, Spr<const Scope> scope){
	const auto type = get_initializer_type(initializer_opt);
	switch(type){
	case Initializer::type_null:
		return bound_result_out.reset();

	case Initializer::type_assignment_init: {
		const auto &candidate = initializer_opt->get<Initializer::S_assignment_init>();
		Vp<Expression> bound_expr;
		bind_expression(bound_expr, candidate.expression, scope);
		Initializer::S_assignment_init init_a = { std::move(bound_expr) };
		return bound_result_out.emplace(std::move(init_a)); }

	case Initializer::type_bracketed_init_list: {
		const auto &candidate = initializer_opt->get<Initializer::S_bracketed_init_list>();
		Vp_vector<Initializer> bound_elems;
		bound_elems.reserve(candidate.elements.size());
		for(const auto &elem : candidate.elements){
			bind_initializer(bound_result_out, elem, scope);
			bound_elems.emplace_back(std::move(bound_result_out));
		}
		Initializer::S_bracketed_init_list init_bracketed = { std::move(bound_elems) };
		return bound_result_out.emplace(std::move(init_bracketed)); }

	case Initializer::type_braced_init_list: {
		const auto &candidate = initializer_opt->get<Initializer::S_braced_init_list>();
		Vp_string_map<Initializer> bound_pairs;
		bound_pairs.reserve(candidate.key_values.size());
		for(const auto &pair : candidate.key_values){
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
void evaluate_initializer(Vp<Reference> &reference_out, Spr<Recycler> recycler, Spr<const Initializer> initializer_opt, Spr<const Scope> scope){
	const auto type = get_initializer_type(initializer_opt);
	switch(type){
	case Initializer::type_null:
		return set_reference(reference_out, nullptr);

	case Initializer::type_assignment_init: {
		const auto &candidate = initializer_opt->get<Initializer::S_assignment_init>();
		return evaluate_expression(reference_out, recycler, candidate.expression, scope); }

	case Initializer::type_bracketed_init_list: {
		const auto &candidate = initializer_opt->get<Initializer::S_bracketed_init_list>();
		Vp<Value> value;
		D_array array;
		array.reserve(candidate.elements.size());
		for(const auto &elem : candidate.elements){
			evaluate_initializer(reference_out, recycler, elem, scope);
			extract_value_from_reference(value, recycler, std::move(reference_out));
			array.emplace_back(std::move(value));
		}
		set_value(value, recycler, std::move(array));
		Reference::S_temporary_value ref_t = { std::move(value) };
		return set_reference(reference_out, std::move(ref_t)); }

	case Initializer::type_braced_init_list: {
		const auto &candidate = initializer_opt->get<Initializer::S_braced_init_list>();
		Vp<Value> value;
		D_object object;
		object.reserve(candidate.key_values.size());
		for(const auto &pair : candidate.key_values){
			evaluate_initializer(reference_out, recycler, pair.second, scope);
			extract_value_from_reference(value, recycler, std::move(reference_out));
			object.emplace(pair.first, std::move(value));
		}
		set_value(value, recycler, std::move(object));
		Reference::S_temporary_value ref_t = { std::move(value) };
		return set_reference(reference_out, std::move(ref_t)); }

	default:
		ASTERIA_DEBUG_LOG("Unknown initializer type enumeration: type = ", type);
		std::terminate();
	}
}

}
