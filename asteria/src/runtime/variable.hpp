// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_HPP_
#define ASTERIA_RUNTIME_VARIABLE_HPP_

#include "../fwd.hpp"
#include "../value.hpp"

namespace asteria {

class Variable final
  : public Rcfwd<Variable>
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

    Variable&
    set_gc_ref(long ref) noexcept
      { this->m_gc_ref = ref;  return *this;  }

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
    open_value()
      { return this->m_value;  }

    // Modifiers
    template<typename XValT>
    Variable&
    initialize(XValT&& xval, State xstat = state_mutable)
      {
        this->m_value = ::std::forward<XValT>(xval);
        this->m_state = xstat;
        return *this;
      }

    Variable&
    uninitialize() noexcept
      {
        this->m_value = ::rocket::sref("[[`dead value`]]");
        this->m_state = state_invalid;
        return *this;
      }
  };

}  // namespace asteria

#endif
