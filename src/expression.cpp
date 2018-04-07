// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "variable.hpp"
#include "reference.hpp"
#include "recycler.hpp"
#include "scope.hpp"
#include "utilities.hpp"
#include <cmath> // isless(), isgreater(), etc.

namespace Asteria {

Expression::Expression(Expression &&) = default;
Expression &Expression::operator=(Expression &&) = default;
Expression::~Expression() = default;

namespace {
	
}

Xptr<Reference> evaluate_expression_recursive_opt(Spref<Recycler> recycler, Spref<Scope> scope, Spref<const Expression> expression_opt){
	(void)recycler;
	(void)scope;
	(void)expression_opt;
	return nullptr;
}

}
