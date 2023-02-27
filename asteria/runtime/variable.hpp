// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_
#define ASTERIA_RUNTIME_VARIABLE_

#include "../fwd.hpp"
#include "../value.hpp"
namespace asteria {

class Variable final
  : public rcfwd<Variable>
  {
  public:
    enum State : uint8_t
      {
        state_invalid     = 0,
        state_immutable  = 1,
        state_mutable    = 2,
      };

  private:
    State m_state = state_invalid;
    Value m_value;
    long m_gc_ref;  // uninitialized by default

  public:
    explicit
    Variable() noexcept
      { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Variable);

    // GC interfaces
    long
    get_gc_ref() const noexcept
      { return this->m_gc_ref;  }

    void
    set_gc_ref(long ref) noexcept
      { this->m_gc_ref = ref;  }

    // Accessors
    State
    state() const noexcept
      { return this->m_state;  }

    bool
    is_uninitialized() const noexcept
      { return this->m_state == state_invalid;  }

    bool
    is_mutable() const noexcept
      { return this->m_state == state_mutable;  }

    const Value&
    get_value() const noexcept
      { return this->m_value;  }

    Value&
    mut_value()
      { return this->m_value;  }

    // Modifiers
    template<typename XValT>
    void
    initialize(XValT&& xval, State xstat = state_mutable)
      {
        this->m_value = ::std::forward<XValT>(xval);
        this->m_state = xstat;
      }

    void
    uninitialize() noexcept
      {
        this->m_value = ::rocket::sref("[[`dead value`]]");
        this->m_state = state_invalid;
      }
  };

}  // namespace asteria
#endif
