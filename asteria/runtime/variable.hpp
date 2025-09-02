// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_
#define ASTERIA_RUNTIME_VARIABLE_

#include "../fwd.hpp"
#include "../value.hpp"
namespace asteria {

class Variable
  :
    public rcfwd<Variable>
  {
  private:
    friend class Garbage_Collector;
    friend class Reference;

    Value m_value;
    bool m_init = false;
    bool m_immutable = false;
    bool m_gc_managed = false;
    int m_gc_ref;  // uninitialized by default

  public:
    Variable()
      noexcept
      { }

  public:
    Variable(const Variable&) = delete;
    Variable& operator=(const Variable&) & = delete;
    ~Variable();

    // accessors
    bool
    initialized()
      const noexcept
      { return this->m_init;  }

    const Value&
    value()
      const noexcept
      { return this->m_value;  }

    Value&
    mut_value()
      { return this->m_value;  }

    template<typename xValue,
    ROCKET_ENABLE_IF(::std::is_assignable<Value&, xValue&&>::value)>
    void
    initialize(xValue&& xval)
      {
        this->m_value = forward<xValue>(xval);
        this->m_init = true;
      }

    void
    uninitialize()
      noexcept
      {
        this->m_value = &"[[`destroyed variable`]]";
        this->m_init = false;
      }

    bool
    immutable()
      const noexcept
      { return this->m_immutable;  }

    void
    set_immutable(bool immut = true)
      noexcept
      { this->m_immutable = immut;  }
  };

}  // namespace asteria
#endif
