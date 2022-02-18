// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_REFERENCE_HPP_
#define ASTERIA_RUNTIME_REFERENCE_HPP_

#include "../fwd.hpp"
#include "../value.hpp"
#include "reference_modifier.hpp"

namespace asteria {

class Reference
  {
  public:
    enum Index : uint8_t
      {
        index_invalid    = 0,
        index_void       = 1,
        index_temporary  = 2,
        index_variable   = 3,
        index_ptc_args   = 4,
      };

  private:
    Value m_value;
    rcfwdp<Variable> m_var;
    rcfwdp<PTC_Arguments> m_ptca;
    cow_vector<Reference_Modifier> m_mods;

    union {
      Index m_index;
      void* m_init_index;  // force initialization of padding bits
    };

  public:
    // Constructors and assignment operators
    constexpr
    Reference() noexcept
      : m_init_index()
      { }

    Reference(const Reference& other) noexcept
      : m_mods(other.m_mods),
        m_index(other.m_index)
      { this->do_copy_partial(other);  }

    Reference(Reference&& other) noexcept
      : m_mods(::std::move(other.m_mods)),
        m_index(other.m_index)
      { this->do_swap_partial(other);  }

    Reference&
    operator=(const Reference& other) noexcept
      { this->do_copy_partial(other);
        this->m_mods = other.m_mods;
        this->m_index = other.m_index;
        return *this;  }

    Reference&
    operator=(Reference&& other) noexcept
      { this->do_swap_partial(other);
        this->m_mods = ::std::move(other.m_mods);
        this->m_index = other.m_index;
        return *this;  }

    Reference&
    swap(Reference& other) noexcept
      { this->do_swap_partial(other);
        this->m_mods.swap(other.m_mods);
        ::std::swap(this->m_index, other.m_index);
        return *this;  }

  private:
    ROCKET_ALWAYS_INLINE void
    do_copy_partial(const Reference& other)
      {
        const uint32_t index = other.m_index;
        if(index == index_temporary)
          this->m_value = other.m_value;
        if(index == index_variable)
          this->m_var = other.m_var;
        if(index == index_ptc_args)
          this->m_ptca = other.m_ptca;
      }

    ROCKET_ALWAYS_INLINE void
    do_swap_partial(Reference& other)
      {
        const bmask32 mask = { this->m_index, other.m_index };
        if(mask.test(index_temporary))
          this->m_value.swap(other.m_value);
        if(mask.test(index_variable))
          this->m_var.swap(other.m_var);
        if(mask.test(index_ptc_args))
          this->m_ptca.swap(other.m_ptca);
      }

    const Value&
    do_dereference_readonly_slow() const;

    Value&
    do_mutate_into_temporary_slow();

    Reference&
    do_finish_call_slow(Global_Context& global);

  public:
    // Accessors
    Index
    index() const noexcept
      { return this->m_index;  }

    bool
    is_invalid() const noexcept
      { return this->index() == index_invalid;  }

    Reference&
    set_invalid() noexcept
      {
        this->m_index = index_invalid;
        return *this;
      }

    bool
    is_void() const noexcept
      { return this->index() == index_void;  }

    Reference&
    set_void() noexcept
      {
        this->m_index = index_void;
        return *this;
      }

    bool
    is_temporary() const noexcept
      { return this->index() == index_temporary;  }

    template<typename XValT>
    Reference&
    set_temporary(XValT&& xval) noexcept
      {
        this->m_value = ::std::forward<XValT>(xval);
        this->m_mods.clear();
        this->m_index = index_temporary;
        return *this;
      }

    bool
    is_variable() const noexcept
      { return this->index() == index_variable;  }

    ASTERIA_INCOMPLET(Variable)
    rcptr<Variable>
    get_variable_opt() const noexcept
      {
        return this->is_variable()
            ? unerase_pointer_cast<Variable>(this->m_var)
            : nullptr;
      }

    ASTERIA_INCOMPLET(Variable)
    Reference&
    set_variable(const rcptr<Variable>& var) noexcept
      {
        this->m_var = var;
        this->m_mods.clear();
        this->m_index = index_variable;
        return *this;
      }

    bool
    is_ptc_args() const noexcept
      { return this->index() == index_ptc_args;  }

    ASTERIA_INCOMPLET(PTC_Arguments)
    rcptr<PTC_Arguments>
    get_ptc_args_opt() const noexcept
      {
        return this->is_ptc_args()
            ? unerase_pointer_cast<PTC_Arguments>(this->m_ptca)
            : nullptr;
      }

    ASTERIA_INCOMPLET(PTC_Arguments)
    Reference&
    set_ptc_args(const rcptr<PTC_Arguments>& ptca) noexcept
      {
        this->m_ptca = ptca;
        this->m_index = index_ptc_args;
        return *this;
      }

    // This is used by garbage collection.
    void
    get_variables(Variable_HashMap& staged, Variable_HashMap& temp) const;

    // A modifier is created by a dot or bracket operator.
    // For instance, the expression `obj.x[42]` results in a reference having two
    // modifiers. Modifiers can be removed to yield references to ancestor objects.
    // Removing the last modifier shall yield the constant `null`.
    const cow_vector<Reference_Modifier>&
    get_modifiers() const noexcept
      { return this->m_mods;  }

    Reference&
    set_modifiers(const cow_vector<Reference_Modifier>& mods) noexcept
      {
        this->m_mods = mods;
        return *this;
      }

    Reference&
    clear_modifiers() noexcept
      {
        this->m_mods.clear();
        return *this;
      }

    Reference&
    push_modifier_array_index(int64_t index)
      {
        Reference_Modifier::S_array_index xmod = { index };
        this->m_mods.emplace_back(::std::move(xmod));
        return *this;
      }

    Reference&
    push_modifier_object_key(const phsh_string& key)
      {
        Reference_Modifier::S_object_key xmod = { key };
        this->m_mods.emplace_back(::std::move(xmod));
        return *this;
      }

    Reference&
    push_modifier_array_head()
      {
        Reference_Modifier::S_array_head xmod = { };
        this->m_mods.emplace_back(::std::move(xmod));
        return *this;
      }

    Reference&
    push_modifier_array_tail()
      {
        Reference_Modifier::S_array_tail xmod = { };
        this->m_mods.emplace_back(::std::move(xmod));
        return *this;
      }

    Reference&
    push_modifier_array_random(uint32_t seed)
      {
        Reference_Modifier::S_array_random xmod = { seed };
        this->m_mods.emplace_back(::std::move(xmod));
        return *this;
      }

    Reference&
    pop_modifier() noexcept
      {
        if(ROCKET_EXPECT(this->m_mods.empty())) {
          // Set to null.
          this->m_value = nullopt;
          this->m_index = index_temporary;
        }
        else {
          // Drop a modifier.
          this->m_mods.pop_back();
        }
        return *this;
      }

    // These are conceptual read/write functions.
    // Some references are placeholders that do not denote values.
    ROCKET_ALWAYS_INLINE const Value&
    dereference_readonly() const
      {
        return ROCKET_EXPECT(this->is_temporary() && this->m_mods.empty())
            ? this->m_value
            : this->do_dereference_readonly_slow();
      }

    ROCKET_ALWAYS_INLINE Value&
    open_temporary()
      {
        return ROCKET_EXPECT(this->is_temporary() && this->m_mods.empty())
            ? this->m_value
            : this->do_mutate_into_temporary_slow();
      }

    ROCKET_ALWAYS_INLINE Reference&
    finish_call(Global_Context& global)
      {
        return ROCKET_EXPECT(!this->is_ptc_args())
            ? *this
            : this->do_finish_call_slow(global);
      }

    Value&
    dereference_mutable() const;

    Value
    dereference_unset() const;
  };

inline void
swap(Reference& lhs, Reference& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
