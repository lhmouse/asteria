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
    rocket::cow_vector<Reference> m_vargs;

  public:
    Variadic_arguer(const Source_location &loc, rocket::cow_vector<Reference> vargs)
      : m_loc(loc), m_vargs(std::move(vargs))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Variadic_arguer);

  public:
    rocket::cow_string describe() const override;
    void enumerate_variables(const Abstract_variable_callback &callback) const override;

    void invoke(Reference &result_out, Global_context &global, Reference &&self, rocket::cow_vector<Reference> &&args) const override;
  };

}

#endif
