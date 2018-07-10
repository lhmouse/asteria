// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "initializer.hpp"
#include "value.hpp"
#include "stored_reference.hpp"
#include "reference.hpp"
#include "expression_node.hpp"
#include "utilities.hpp"

namespace Asteria {

Initializer::Initializer(Initializer &&) noexcept = default;
Initializer & Initializer::operator=(Initializer &&) noexcept = default;
Initializer::~Initializer() = default;

Initializer bind_initializer(const Initializer &init, Sp_cref<const Scope> scope){
	// Bind elements recursively.
	const auto type = init.which();
	switch(type){
	case Initializer::index_assignment_init: {
		const auto &cand = init.as<Initializer::S_assignment_init>();
		auto bound_expr = bind_expression(cand.expr, scope);
		Initializer::S_assignment_init init_ai = { std::move(bound_expr) };
		return std::move(init_ai); }

	case Initializer::index_bracketed_init_list: {
		const auto &cand = init.as<Initializer::S_bracketed_init_list>();
		Vector<Initializer> bound_elems;
		bound_elems.reserve(cand.elems.size());
		for(const auto &elem : cand.elems){
			auto bound_init = bind_initializer(elem, scope);
			bound_elems.emplace_back(std::move(bound_init));
		}
		Initializer::S_bracketed_init_list init_brkt = { std::move(bound_elems) };
		return std::move(init_brkt); }

	case Initializer::index_braced_init_list: {
		const auto &cand = init.as<Initializer::S_braced_init_list>();
		Dictionary<Initializer> bound_pairs;
		bound_pairs.reserve(cand.pairs.size());
		for(const auto &pair : cand.pairs){
			auto bound_init = bind_initializer(pair.second, scope);
			bound_pairs.emplace(pair.first, std::move(bound_init));
		}
		Initializer::S_braced_init_list init_brac = { std::move(bound_pairs) };
		return std::move(init_brac); }

	default:
		ASTERIA_DEBUG_LOG("An unknown initializer type enumeration: type = ", type, "` is encountered. This is probably a bug. Please report.");
		std::terminate();
	}
}
void evaluate_initializer(Vp<Reference> &result_out, const Initializer &init, Sp_cref<const Scope> scope){
	const auto type = init.which();
	switch(type){
	case Initializer::index_assignment_init: {
		const auto &cand = init.as<Initializer::S_assignment_init>();
		evaluate_expression(result_out, cand.expr, scope);
		return; }

	case Initializer::index_bracketed_init_list: {
		const auto &cand = init.as<Initializer::S_bracketed_init_list>();
		Value value;
		D_array array;
		array.reserve(cand.elems.size());
		for(const auto &elem : cand.elems){
			evaluate_initializer(result_out, elem, scope);
			extract_value_from_reference(value, std::move(result_out));
			array.emplace_back(std::move(value));
		}
		set_value(value, std::move(array));
		Reference::S_temporary_value ref_t = { std::move(value) };
		set_reference(result_out, std::move(ref_t));
		return; }

	case Initializer::index_braced_init_list: {
		const auto &cand = init.as<Initializer::S_braced_init_list>();
		Value value;
		D_object object;
		object.reserve(cand.pairs.size());
		for(const auto &pair : cand.pairs){
			evaluate_initializer(result_out, pair.second, scope);
			extract_value_from_reference(value, std::move(result_out));
			object.emplace(pair.first, std::move(value));
		}
		set_value(value, std::move(object));
		Reference::S_temporary_value ref_t = { std::move(value) };
		set_reference(result_out, std::move(ref_t));
		return; }

	default:
		ASTERIA_DEBUG_LOG("An unknown initializer type enumeration: type = ", type, "` is encountered. This is probably a bug. Please report.");
		std::terminate();
	}
}

}
