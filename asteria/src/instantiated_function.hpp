// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_INSTANTIATED_FUNCTION_HPP_

#include "fwd.hpp"
#include "abstract_function.hpp"
#include "function_header.hpp"
#include "variadic_arguer.hpp"
#include "block.hpp"

namespace Asteria {

class Instantiated_function : public Abstract_function
  {
  private:
    Function_header m_head;
    Shared_function_wrapper m_zvarg;
    Block m_body_bnd;

  public:
    Instantiated_function(const Function_header &head, Block body_bnd)
      : m_head(head), m_zvarg(Variadic_arguer(head.get_location(), { })), m_body_bnd(std::move(body_bnd))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Instantiated_function);

  public:
    String describe() const override;
    void enumerate_variables(const Abstract_variable_callback &callback) const override;

    Reference invoke(Global_context &global, Reference self, Vector<Reference> args) const override;
  };

}

#endif
