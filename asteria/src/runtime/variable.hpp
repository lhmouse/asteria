// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_VARIABLE_HPP_
#define ASTERIA_RUNTIME_VARIABLE_HPP_

#include "../fwd.hpp"
#include "../value.hpp"

namespace asteria {

class Variable
  final
  : public Rcfwd<Variable>
  {
  private:
    Value m_value;
    bool m_immut = false;
    bool m_valid = false;

    // These are fields for garbage collection and are uninitialized by
    // default. Because values are reference-counted, it is possible for
    // a variable to be encountered multiple times at the marking stage
    // during garbage collection. It is essential that we mark the value
    // exactly once.
    bool m_gc_mark;
    long m_gc_ref;

  public:
    explicit
    Variable()
      noexcept
      { }

  public:
    ASTERIA_NONCOPYABLE_DESTRUCTOR(Variable);

    const Value&
    get_value()
      const noexcept
      { return this->m_value;  }

    Value&
    open_value()
      { return this->m_value;  }

    bool
    is_immutable()
      const noexcept
      { return this->m_immut;  }

    Variable&
    set_immutable(bool immutable)
      noexcept
      { return this->m_immut = immutable, *this;  }

    bool
    is_initialized()
      const noexcept
      { return this->m_valid;  }

    template<typename XValT>
    Variable&
    initialize(XValT&& xval, bool immut)
      {
        this->m_value = ::std::forward<XValT>(xval);
        this->m_immut = immut;
        this->m_valid = true;
        return *this;
      }

    Variable&
    uninitialize()
      noexcept
      {
        this->m_value = INT64_C(0x6EEF8BADF00DDEAD);
        this->m_immut = true;
        this->m_valid = false;
        return *this;
      }

    long
    get_gc_ref()
      const noexcept
      { return this->m_gc_ref;  }

    Variable&
    reset_gc_ref(long ref)
      noexcept
      {
        this->m_gc_mark = false;
        this->m_gc_ref = ref;
        return *this;
      }

    bool
    add_gc_ref()
      noexcept
      {
        bool m = ::std::exchange(this->m_gc_mark, true);
        this->m_gc_ref += 1;
        return m;
      }
  };

}  // namespace asteria

#endif
