// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_VARIADIC_ARGUER_HPP_
#define ASTERIA_VARIADIC_ARGUER_HPP_

#include "fwd.hpp"
#include "abstract_function.hpp"
#include "source_location.hpp"
#include "reference.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Variadic_arguer : public Abstract_function
  {
  private:
    Source_location m_loc;
    Vector<Reference> m_vargs;

  public:
    Variadic_arguer(const Source_location &loc, Vector<Reference> vargs)
      : m_loc(loc), m_vargs(std::move(vargs))
      {
      }
    ~Variadic_arguer();

  public:
    String describe() const override;
    void enumerate_variables(const Abstract_variable_callback &callback) const override;

    Reference invoke(Global_context &global, Reference self, Vector<Reference> args) const override;
  };

}

#endif
