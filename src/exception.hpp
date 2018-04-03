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
	Reference m_reference;

public:
	explicit Exception(Reference reference)
		: m_reference(std::move(reference))
	{ }
	Exception(Exception &&);
	Exception &operator=(Exception &&);
	~Exception() override;

public:
	const char *what() const noexcept override {
		return "Asteria::Exception";
	}

	const Reference &get_reference() const noexcept {
		return m_reference;
	}
	Reference &get_reference() noexcept {
		return m_reference;
	}
	void set_reference(Reference &&reference){
		m_reference = std::move(reference);
	}
};

}

#endif
