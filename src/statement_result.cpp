// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "statement_result.hpp"
#include "utilities.hpp"

namespace Asteria {

Statement_result::Statement_result(Statement_result &&) noexcept = default;
Statement_result &Statement_result::operator=(Statement_result &&) = default;
Statement_result::~Statement_result() = default;

}
