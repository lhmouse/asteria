// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_PLACEHOLDER_HPP_
#define ASTERIA_RUNTIME_PLACEHOLDER_HPP_

#include "../fwd.hpp"
#include "abstract_opaque.hpp"
#include "abstract_function.hpp"

namespace Asteria {

class Placeholder : public Abstract_Opaque, public Abstract_Function
  {
  public:
    Placeholder() noexcept
      {
      }
    ~Placeholder() override;

  public:
    std::ostream& describe(std::ostream& os) const override;
    [[noreturn]] Reference& invoke(Reference& self, const Global_Context& global, Cow_Vector<Reference>&& args) const override;
    void enumerate_variables(const Abstract_Variable_Callback& callback) const override;
  };

}  // namespace Asteria

#endif
