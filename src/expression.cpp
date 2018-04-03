// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "variable.hpp"
#include "reference.hpp"
#include "recycler.hpp"
#include "scope.hpp"
#include "utilities.hpp"

namespace Asteria {

Expression::Expression(Expression &&) = default;
Expression &Expression::operator=(Expression &&) = default;
Expression::~Expression() = default;

Reference evaluate_expression_recursive(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt){
	(void)recycler;
	(void)scope;
	(void)expression_opt;
	Reference::Rvalue_generic ref = { nullptr };
	return ref;
}
/*
Reference evaluate_expression_recursive(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt){
	if(!expression_opt){
		// Return a null reference.
		Reference::Rvalue_generic ref = { nullptr };
		return Reference(recycler, std::move(ref));
	} else {
		boost::container::vector<Reference> evaluation_stack;
		for(std::size_t i = 0; i < expression_opt->size(); ++i){
			const auto type = expression_opt->get_type_at(i);
			switch(type){
			case Expression::type_literal_generic: {
				const auto &params = expression_opt->get_at<Literla_generic>();
				evaluation_stack.emplace_back(
		}
	}
}
*/
}
