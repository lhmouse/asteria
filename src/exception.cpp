// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "exception.hpp"

namespace Asteria {

Exception::Exception(const Exception &) noexcept = default;
Exception &Exception::operator=(const Exception &) noexcept = default;
Exception::Exception(Exception &&) noexcept = default;
Exception &Exception::operator=(Exception &&) noexcept = default;
Exception::~Exception() = default;

}
