// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_TAIL_CALL_ARGUMENTS_HPP_
#define ASTERIA_RUNTIME_TAIL_CALL_ARGUMENTS_HPP_

#include "../fwd.hpp"
#include "../source_location.hpp"
#include "../abstract_function.hpp"

namespace Asteria {

class Tail_Call_Arguments final : public virtual Rcbase
  {
  private:
    // These describe characteristics of the function call.
    Source_Location m_sloc;  // location of the call
    cow_string m_inside;  // signature of the enclosing function
    Source_Location m_insloc;  // location of the enclosing function
    PTC_Aware m_ptc;

    // This is the target function.
    ckptr<Abstract_Function> m_target;
    // The last reference is `self`.
    // We can't store a `Reference` directly as the class `Reference` is incomplete here.
    cow_vector<Reference> m_args_self;

  public:
    Tail_Call_Arguments(const Source_Location& sloc, const cow_string& inside, const Source_Location& insloc,
                        PTC_Aware ptc, const ckptr<Abstract_Function>& target, cow_vector<Reference>&& args_self)
      :
        m_sloc(sloc), m_inside(inside), m_insloc(insloc),
        m_ptc(ptc), m_target(target), m_args_self(::rocket::move(args_self))
      {
      }
    ~Tail_Call_Arguments() override;

    Tail_Call_Arguments(const Tail_Call_Arguments&)
      = delete;
    Tail_Call_Arguments& operator=(const Tail_Call_Arguments&)
      = delete;

  public:
    const Source_Location& source_location() const noexcept
      {
        return this->m_sloc;
      }
    const cow_string& inside() const noexcept
      {
        return this->m_inside;
      }
    const Source_Location& enclosing_function_location() const noexcept
      {
        return this->m_insloc;
      }
    PTC_Aware ptc_aware() const noexcept
      {
        return this->m_ptc;
      }

    const ckptr<Abstract_Function>& get_target() const noexcept
      {
        return this->m_target;
      }
    const cow_vector<Reference>& get_arguments_and_self() const noexcept
      {
        return this->m_args_self;
      }
    cow_vector<Reference>& open_arguments_and_self() noexcept
      {
        return this->m_args_self;
      }

    Variable_Callback& enumerate_variables(Variable_Callback& callback) const;
  };

}  // namespace Asteria

#endif
