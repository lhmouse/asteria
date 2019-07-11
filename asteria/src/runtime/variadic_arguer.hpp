// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIADIC_ARGUER_HPP_
#define ASTERIA_RUNTIME_VARIADIC_ARGUER_HPP_

#include "../fwd.hpp"
#include "abstract_function.hpp"
#include "reference.hpp"
#include "../syntax/source_location.hpp"

namespace Asteria {

class Variadic_Arguer : public Abstract_Function
  {
  private:
    Source_Location m_sloc;
    PreHashed_String m_func;
    Cow_Vector<Reference> m_vargs;

  public:
    template<typename... XvargsT> Variadic_Arguer(const Source_Location& sloc, const PreHashed_String& func,
                                                  XvargsT&&... xvargs)
      : m_sloc(sloc), m_func(func),
        m_vargs(rocket::forward<XvargsT>(xvargs)...)
      {
      }
    template<typename... XvargsT> Variadic_Arguer(const Variadic_Arguer& other,
                                                  XvargsT&&... xvargs)
      : m_sloc(other.m_sloc), m_func(other.m_func),
        m_vargs(rocket::forward<XvargsT>(xvargs)...)
      {
      }

  public:
    const Source_Location& get_source_location() const noexcept
      {
        return this->m_sloc;
      }
    const Cow_String& get_source_file() const noexcept
      {
        return this->m_sloc.file();
      }
    std::uint32_t get_source_line() const noexcept
      {
        return this->m_sloc.line();
      }
    const PreHashed_String& get_function_signature() const noexcept
      {
        return this->m_func;
      }
    std::size_t count_arguments() const noexcept
      {
        return this->m_vargs.size();
      }
    const Reference& get_argument(std::size_t index) const
      {
        return this->m_vargs.at(index);
      }

    std::ostream& describe(std::ostream& os) const override;
    Reference& invoke(Reference& self, const Global_Context& global, Cow_Vector<Reference>&& args) const override;
    void enumerate_variables(const Abstract_Variable_Callback& callback) const override;
  };

}  // namespace Asteria

#endif
