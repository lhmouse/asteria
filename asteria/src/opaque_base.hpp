// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_OPAQUE_BASE_HPP_
#define ASTERIA_OPAQUE_BASE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Opaque_base {
private:
	const String m_description;

public:
	explicit Opaque_base(String description)
		: m_description(std::move(description))
	{ }
	virtual ~Opaque_base();

	Opaque_base(const Opaque_base &) = delete;
	Opaque_base & operator=(const Opaque_base &) = delete;

public:
	const String & describe() const noexcept {
		return m_description;
	}
};

}

#endif
