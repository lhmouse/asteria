// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_FUNCTION_BASE_HPP_
#define ASTERIA_FUNCTION_BASE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Function_base {
public:
	Function_base() = default;
	virtual ~Function_base();

	Function_base(const Function_base &) = delete;
	Function_base &operator=(const Function_base &) = delete;

public:
	virtual const char *describe() const noexcept = 0;
	virtual void invoke(Xptr<Reference> &result_out, Spcref<Recycler> recycler, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt) const = 0;
};

}

#endif
