// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "exception.hpp"

namespace Asteria {

Exception::Exception(Exception &&) noexcept = default;
Exception &Exception::operator=(Exception &&) noexcept = default;
Exception::~Exception() = default;

const char *Exception::what() const noexcept {
	return "Asteria::Exception";
}

}
