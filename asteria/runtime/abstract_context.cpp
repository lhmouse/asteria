// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "abstract_context.hpp"
#include "../utils.hpp"
namespace asteria {

Reference*
Abstract_Context::
do_create_lazy_reference_opt(Reference* hint_opt, phsh_stringR /*name*/) const
  {
    return hint_opt;
  }

}  // namespace asteria
