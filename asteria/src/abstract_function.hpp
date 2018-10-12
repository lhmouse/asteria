// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_FUNCTION_HPP_
#define ASTERIA_ABSTRACT_FUNCTION_HPP_

#include "fwd.hpp"

namespace Asteria {

class Abstract_function : public Rcbase<Abstract_function>
  {
  public:
    Abstract_function() noexcept
      {
      }
    virtual ~Abstract_function();

  public:
    virtual String describe() const = 0;
    virtual void collect_variables(bool (*callback)(void *, const Rcptr<Variable> &), void *param) const = 0;

    virtual Reference invoke(Global_context &global, Reference self, Vector<Reference> args) const = 0;
  };

}

#endif
