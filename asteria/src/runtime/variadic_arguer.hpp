// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIADIC_ARGUER_HPP_
#define ASTERIA_RUNTIME_VARIADIC_ARGUER_HPP_

#include "../fwd.hpp"
#include "abstract_function.hpp"
#include "reference.hpp"
#include "../syntax/source_location.hpp"
#include "../rocket/refcounted_ptr.hpp"

namespace Asteria {

class Variadic_arguer : public Abstract_function
  {
  private:
    Source_location m_loc;
    rocket::prehashed_string m_name;
    rocket::cow_vector<Reference> m_vargs;

  public:
    template<typename ...XvargsT>
      Variadic_arguer(const Source_location &loc, const rocket::prehashed_string &name, XvargsT &&...xvargs)
      : m_loc(loc), m_name(name), m_vargs(std::forward<XvargsT>(xvargs)...)
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Variadic_arguer);

  public:
    rocket::cow_string describe() const override;
    void enumerate_variables(const Abstract_variable_callback &callback) const override;

    void invoke(Reference &self_io, Global_context &global, rocket::cow_vector<Reference> &&args) const override;
  };

}

#endif
