// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "statement.hpp"
#include "utilities.hpp"

namespace Asteria {

Statement::Statement(Statement &&) noexcept = default;
Statement &Statement::operator=(Statement &&) = default;
Statement::~Statement() = default;

Statement::Type get_statement_type(Spcref<const Statement> statement_opt) noexcept {
	return statement_opt ? statement_opt->get_type() : Statement::type_null;
}

}
