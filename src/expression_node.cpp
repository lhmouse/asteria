// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "expression_node.hpp"
#include "variable.hpp"

namespace Asteria {

Expression_node::Expression_node(Expression_node &&) = default;
Expression_node &Expression_node::operator=(Expression_node &&) = default;
Expression_node::~Expression_node() = default;

}
