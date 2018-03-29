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

Reference Expression::evaluate(const Shared_ptr<Recycler> &recycler, const Shared_ptr<Scope> &scope) const {
	ASTERIA_DEBUG_LOG("NOT IMPLEMENTED YET");
	(void)recycler;
	(void)scope;
	Reference::Rvalue_generic ref = { std::make_shared<Variable>(std::string("hello")) };
	return Reference(std::move(ref));
}

}
