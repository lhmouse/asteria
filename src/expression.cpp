// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "variable.hpp"
#include "reference.hpp"
#include "misc.hpp"

namespace Asteria {

Expression::Expression(Expression &&) = default;
Expression &Expression::operator=(Expression &&) = default;
Expression::~Expression() = default;

Reference evaluate_expression(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt){
	ASTERIA_DEBUG_LOG("NOT IMPLEMENTED YET");
	(void)recycler;
	(void)scope;
	(void)expression_opt;
	Reference::Rvalue_generic ref = { create_shared<Variable>(std::string("hello")) };
	return Reference(recycler, std::move(ref));
}

}
