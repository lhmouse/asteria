// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "compound_statement.hpp"
#include "statement.hpp"
#include "utilities.hpp"

namespace Asteria {

Compound_statement::Compound_statement(Compound_statement &&) noexcept = default;
Compound_statement &Compound_statement::operator=(Compound_statement &&) = default;
Compound_statement::~Compound_statement() = default;

}
