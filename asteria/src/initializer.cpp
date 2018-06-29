// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "initializer.hpp"
#include "value.hpp"
#include "stored_reference.hpp"
#include "reference.hpp"
#include "expression.hpp"
#include "utilities.hpp"

namespace Asteria {

Initializer::Initializer(Initializer &&) noexcept = default;
Initializer & Initializer::operator=(Initializer &&) noexcept = default;
Initializer::~Initializer() = default;

void bind_initializer(Vp<Initializer> &bound_init_out, Spr<const Initializer> init_opt, Spr<const Scope> scope){
	if(init_opt == nullptr){
		// Return a null initializer.
		bound_init_out.reset();
		return;
	}
	const auto type = init_opt->get_type();
	switch(type){
	case Initializer::type_assignment_init: {
		const auto &cand = init_opt->get<Initializer::S_assignment_init>();
		Vp<Expression> bound_expr;
		bind_expression(bound_expr, cand.expr, scope);
		Initializer::S_assignment_init init_a = { std::move(bound_expr) };
		bound_init_out.emplace(std::move(init_a));
		break; }

	case Initializer::type_bracketed_init_list: {
		const auto &cand = init_opt->get<Initializer::S_bracketed_init_list>();
		Vector<Vp<Initializer>> bound_elems;
		bound_elems.reserve(cand.elems.size());
		for(const auto &elem : cand.elems){
			bind_initializer(bound_init_out, elem, scope);
			bound_elems.emplace_back(std::move(bound_init_out));
		}
		Initializer::S_bracketed_init_list init_bracketed = { std::move(bound_elems) };
		bound_init_out.emplace(std::move(init_bracketed));
		break; }

	case Initializer::type_braced_init_list: {
		const auto &cand = init_opt->get<Initializer::S_braced_init_list>();
		Dictionary<Vp<Initializer>> bound_pairs;
		bound_pairs.reserve(cand.key_values.size());
		for(const auto &pair : cand.key_values){
			bind_initializer(bound_init_out, pair.second, scope);
			bound_pairs.emplace(pair.first, std::move(bound_init_out));
		}
		Initializer::S_braced_init_list init_braced = { std::move(bound_pairs) };
		bound_init_out.emplace(std::move(init_braced));
		break; }

	default:
		ASTERIA_DEBUG_LOG("An unknown initializer type enumeration: type = ", type, "` is encountered. This is probably a bug. Please report.");
		std::terminate();
	}
}
void evaluate_initializer(Vp<Reference> &result_out, Spr<Recycler> recycler_out, Spr<const Initializer> init_opt, Spr<const Scope> scope){
	if(init_opt == nullptr){
		// Return a null reference only when a null initializer is given.
		move_reference(result_out, nullptr);
		return;
	}
	const auto type = init_opt->get_type();
	switch(type){
	case Initializer::type_assignment_init: {
		const auto &cand = init_opt->get<Initializer::S_assignment_init>();
		evaluate_expression(result_out, recycler_out, cand.expr, scope);
		break; }

	case Initializer::type_bracketed_init_list: {
		const auto &cand = init_opt->get<Initializer::S_bracketed_init_list>();
		Vp<Value> value;
		D_array array;
		array.reserve(cand.elems.size());
		for(const auto &elem : cand.elems){
			evaluate_initializer(result_out, recycler_out, elem, scope);
			extract_value_from_reference(value, recycler_out, std::move(result_out));
			array.emplace_back(std::move(value));
		}
		set_value(value, recycler_out, std::move(array));
		Reference::S_temporary_value ref_t = { std::move(value) };
		set_reference(result_out, std::move(ref_t));
		break; }

	case Initializer::type_braced_init_list: {
		const auto &cand = init_opt->get<Initializer::S_braced_init_list>();
		Vp<Value> value;
		D_object object;
		object.reserve(cand.key_values.size());
		for(const auto &pair : cand.key_values){
			evaluate_initializer(result_out, recycler_out, pair.second, scope);
			extract_value_from_reference(value, recycler_out, std::move(result_out));
			object.emplace(pair.first, std::move(value));
		}
		set_value(value, recycler_out, std::move(object));
		Reference::S_temporary_value ref_t = { std::move(value) };
		set_reference(result_out, std::move(ref_t));
		break; }

	default:
		ASTERIA_DEBUG_LOG("An unknown initializer type enumeration: type = ", type, "` is encountered. This is probably a bug. Please report.");
		std::terminate();
	}
}

}
