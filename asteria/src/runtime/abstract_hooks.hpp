// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_ABSTRACT_HOOKS_HPP_
#define ASTERIA_RUNTIME_ABSTRACT_HOOKS_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"

namespace Asteria {

class Abstract_Hooks : public virtual Rcbase
  {
  public:
    Abstract_Hooks() noexcept
      {
      }
    ~Abstract_Hooks() override;

  public:
    virtual void on_variable_declare(const Source_Location& sloc, const phsh_string& name);

    virtual void on_function_call(const Source_Location& sloc, const phsh_string& inside);
    virtual void on_function_return(const Source_Location& sloc, const phsh_string& inside) noexcept;
    virtual void on_function_except(const Source_Location& sloc, const phsh_string& inside, const Exception& except) noexcept;
  };

}  // namespace Asteria

#endif
