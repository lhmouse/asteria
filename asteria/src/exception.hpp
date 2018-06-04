// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXCEPTION_HPP_
#define ASTERIA_EXCEPTION_HPP_

#include "fwd.hpp"
#include "reference.hpp"
#include <exception>

namespace Asteria {

class Exception : public virtual std::exception, public virtual std::nested_exception {
private:
	Sp<const Reference> m_reference_opt;

public:
	explicit Exception(Sp<const Reference> &&reference_opt) noexcept
		: m_reference_opt(std::move(reference_opt))
	{ }
	Exception(const Exception &) noexcept;
	Exception & operator=(const Exception &) noexcept;
	Exception(Exception &&) noexcept;
	Exception & operator=(Exception &&) noexcept;
	~Exception() override;

public:
	const Sp<const Reference> & get_reference_opt() const noexcept {
		return m_reference_opt;
	}
	const char * what() const noexcept override;
};

}

#endif
