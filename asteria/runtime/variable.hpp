// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

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
    Value m_value;
    bool m_init = false;
    bool m_immut = false;
    int m_gc_ref;  // uninitialized by default

  public:
    explicit
    Variable() noexcept = default;

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Variable);

    // Accessors
    bool
    is_initialized() const noexcept
      { return this->m_init;  }

    const Value&
    get_value() const noexcept
      { return this->m_value;  }

    Value&
    mut_value()
      { return this->m_value;  }

    template<typename XValT,
    ROCKET_ENABLE_IF(::std::is_assignable<Value&, XValT&&>::value)>
    void
    initialize(XValT&& xval)
      {
        this->m_value = forward<XValT>(xval);
        this->m_init = true;
      }

    void
    uninitialize() noexcept
      {
        this->m_value = sref("[[`destroyed variable`]]");
        this->m_init = false;
      }

    bool
    is_immutable() const noexcept
      { return this->m_immut;  }

    void
    set_immutable(bool immut = true) noexcept
      { this->m_immut = immut;  }

    // GC interfaces
    int
    get_gc_ref() const noexcept
      { return this->m_gc_ref;  }

    void
    set_gc_ref(int ref) noexcept
      { this->m_gc_ref = ref;  }
  };

}  // namespace asteria
#endif
