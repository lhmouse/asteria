// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "statement.hpp"
#include "utilities.hpp"

namespace Asteria {

Statement::Statement(Statement &&) = default;
Statement &Statement::operator=(Statement &&) = default;
Statement::~Statement() = default;

}
