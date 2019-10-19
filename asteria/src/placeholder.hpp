// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_PLACEHOLDER_HPP_
#define ASTERIA_PLACEHOLDER_HPP_

#include "fwd.hpp"
#include "abstract_opaque.hpp"
#include "abstract_function.hpp"

namespace Asteria {

class Placeholder final : public Abstract_Opaque, public Abstract_Function
  {
  public:
    Placeholder() noexcept
      {
      }
    ~Placeholder() override;

  public:
    tinyfmt& describe(tinyfmt& fmt) const override;
    [[noreturn]] Reference& invoke(Reference& self, const Global_Context& global, cow_vector<Reference>&& args) const override;
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const override;
  };

}  // namespace Asteria

#endif
