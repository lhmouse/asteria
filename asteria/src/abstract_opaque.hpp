// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_ABSTRACT_OPAQUE_HPP_
#define ASTERIA_ABSTRACT_OPAQUE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Abstract_opaque : public Rcbase<Abstract_opaque>
  {
  public:
    Abstract_opaque() noexcept
      {
      }
    virtual ~Abstract_opaque();

  public:
    virtual String describe() const = 0;
    virtual Abstract_opaque * clone(Rcptr<Abstract_opaque> &value_out) const = 0;
    virtual void collect_variables(bool (*callback)(void *, const Rcptr<Variable> &), void *param) const = 0;

    virtual const Value * get_member_opt(const String &key) const = 0;
    virtual Value * get_member_opt(const String &key) = 0;
    virtual Value & open_member(const String &key) = 0;
    virtual void unset_member(const String &key) = 0;
  };

}

#endif
