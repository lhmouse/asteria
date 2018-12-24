// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_
#define ASTERIA_RUNTIME_INSTANTIATED_FUNCTION_HPP_

#include "../fwd.hpp"
#include "abstract_function.hpp"
#include "../syntax/source_location.hpp"
#include "../syntax/block.hpp"
#include "../rocket/cow_vector.hpp"

namespace Asteria {

class Instantiated_function : public Abstract_function
  {
  private:
    Source_location m_loc;
    rocket::prehashed_string m_name;
    rocket::cow_vector<rocket::prehashed_string> m_params;
    Block m_body_bnd;

  public:
    Instantiated_function(const Source_location &loc, const rocket::prehashed_string &name, const rocket::cow_vector<rocket::prehashed_string> &params, Block body_bnd)
      : m_loc(loc), m_name(name), m_params(params), m_body_bnd(std::move(body_bnd))
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Instantiated_function);

  public:
    rocket::cow_string describe() const override;
    void invoke(Reference &self_io, Global_context &global, rocket::cow_vector<Reference> &&args) const override;
    void enumerate_variables(const Abstract_variable_callback &callback) const override;
  };

}

#endif
