// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_EXCEPTION_HPP_
#define ASTERIA_EXCEPTION_HPP_

#include <exception>
#include "fwd.hpp"
#include "reference.hpp"

namespace Asteria {

class Exception : public std::exception {
private:
	std::shared_ptr<Reference> m_reference_opt;

public:
	explicit Exception(std::shared_ptr<Reference> reference_opt)
		: m_reference_opt(std::move(reference_opt))
	{ }
	~Exception() override;

public:
	const char *what() const noexcept override;

	Reference get_reference() const;
	void set_reference(Reference reference);
	void clear_reference() noexcept;
};

}

#endif
