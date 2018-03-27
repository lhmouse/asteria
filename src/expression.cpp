// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression.hpp"
#include "variable.hpp"
#include "reference.hpp"
#include "misc.hpp"

namespace Asteria {

Expression::~Expression(){
	//
}

Reference Expression::evaluate() const {
	ASTERIA_DEBUG_LOG("NOT IMPLEMENTED YET");
	return Reference(Reference::Direct_reference(std::make_shared<Variable>(std::string("hello"))));
}

}
