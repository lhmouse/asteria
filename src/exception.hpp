// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXCEPTION_HPP_
#define ASTERIA_EXCEPTION_HPP_

#include "fwd.hpp"
#include "reference.hpp"
#include <exception>

namespace Asteria {

class Exception : public std::exception {
private:
	Sptr<Reference> m_reference_opt;

public:
	explicit Exception(Sptr<Reference> reference_opt) noexcept
		: m_reference_opt(std::move(reference_opt))
	{ }
	Exception(const Exception &) noexcept;
	Exception &operator=(const Exception &) noexcept;
	Exception(Exception &&) noexcept;
	Exception &operator=(Exception &&) noexcept;
	~Exception() override;

public:
	const char *what() const noexcept override;

	Sptr<const Reference> get_reference_opt() const noexcept {
		return m_reference_opt;
	}
	Sptr<Reference> get_reference_opt() noexcept {
		return m_reference_opt;
	}
};

}

#endif
