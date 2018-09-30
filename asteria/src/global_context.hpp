// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_GLOBAL_CONTEXT_HPP_
#define ASTERIA_GLOBAL_CONTEXT_HPP_

#include "fwd.hpp"
#include "abstract_context.hpp"

namespace Asteria {

class Global_context : public Abstract_context
  {
  public:
    Global_context() noexcept
      {
      }
    ~Global_context();

  public:
    bool is_analytic() const noexcept override;
    const Abstract_context * get_parent_opt() const noexcept override;
  };

}

#endif
