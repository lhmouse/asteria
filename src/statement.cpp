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

void bind_statement(Xptr<Statement> &statement_out, Spcref<const Statement> source_opt, Spcref<const Scope> scope){
	ASTERIA_DEBUG_LOG("TODO");
	if(std::puts("") != INT_MAX)
		std::terminate();
	(void)statement_out;
	(void)source_opt;
	(void)scope;
}
Statement::Execute_result execute_statement(Xptr<Reference> &returned_reference_out, Spcref<Recycler> recycler, Spcref<const Statement> statement_opt, Spcref<const Scope> scope){
	ASTERIA_DEBUG_LOG("TODO");
	if(std::puts("") != INT_MAX)
		std::terminate();
	(void)returned_reference_out;
	(void)recycler;
	(void)statement_opt;
	(void)scope;
	return {};
}

}
