// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "variable.hpp"
#include "misc.hpp"

namespace Asteria {

Expression::~Expression(){
	//
}

Value_ptr<Variable> Expression::evaluate() const {
	ASTERIA_DEBUG_LOG("NOT IMPLEMENTED YET");
	return make_value<Variable>(std::string("hello"));
}

}
