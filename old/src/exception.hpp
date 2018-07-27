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
	Reference m_ref;

public:
	explicit Exception(Reference &&ref) noexcept
		: m_ref(std::move(ref))
	{ }
	Exception(Exception &&) noexcept;
	Exception & operator=(Exception &&) noexcept;
	~Exception() override;

public:
	const Reference & get_reference() const noexcept
	{
		return m_ref;
	}
	void set_reference(Reference &&ref) noexcept
	{
		m_ref = std::move(ref);
	}

	const char * what() const noexcept override;
};

}

#endif
