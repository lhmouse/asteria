// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIADIC_ARGUER_HPP_
#define ASTERIA_RUNTIME_VARIADIC_ARGUER_HPP_

#include "../fwd.hpp"
#include "abstract_function.hpp"
#include "reference.hpp"
#include "../syntax/source_location.hpp"
#include "../rocket/cow_vector.hpp"

namespace Asteria {

class Variadic_Arguer : public Abstract_Function
  {
  private:
    Source_Location m_loc;
    rocket::prehashed_string m_name;
    rocket::cow_vector<Reference> m_vargs;

  public:
    template<typename ...XvargsT>
      Variadic_Arguer(const Source_Location &loc, const rocket::prehashed_string &name, XvargsT &&...xvargs)
      : m_loc(loc), m_name(name), m_vargs(std::forward<XvargsT>(xvargs)...)
      {
      }
    template<typename XfirstT, typename ...XvargsT>
      Variadic_Arguer(const Variadic_Arguer &other, XfirstT &&xfirst, XvargsT &&...xvargs)
      : m_loc(other.m_loc), m_name(other.m_name), m_vargs(std::forward<XfirstT>(xfirst), std::forward<XvargsT>(xvargs)...)
      {
      }
    ROCKET_COPYABLE_DESTRUCTOR(Variadic_Arguer);

  public:
    const Source_Location & get_location() const noexcept
      {
        return this->m_loc;
      }
    const rocket::prehashed_string & get_name() const noexcept
      {
        return this->m_name;
      }
    std::size_t get_varg_size() const noexcept
      {
        return this->m_vargs.size();
      }
    const Reference & get_varg(std::size_t index) const
      {
        return this->m_vargs.at(index);
      }

    rocket::cow_string describe() const override;
    void invoke(Reference &self_io, Global_Context &global, rocket::cow_vector<Reference> &&args) const override;
    void enumerate_variables(const Abstract_Variable_Callback &callback) const override;
  };

}

#endif
