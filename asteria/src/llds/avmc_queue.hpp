// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_HPP_
#define ASTERIA_LLDS_AVMC_QUEUE_HPP_

#include "../fwd.hpp"
#include "../source_location.hpp"
#include "../details/avmc_queue.ipp"

namespace asteria {

class AVMC_Queue
  {
  public:
    using Uparam      = details_avmc_queue::Uparam;
    using Symbols     = details_avmc_queue::Symbols;
    using Executor    = details_avmc_queue::Executor;
    using Enumerator  = details_avmc_queue::Enumerator;

  private:
    using Header       = details_avmc_queue::Header;
    using Constructor  = details_avmc_queue::Constructor;
    using Vtable       = details_avmc_queue::Vtable;

    Header* m_bptr = nullptr;  // beginning of raw storage
    uint32_t m_rsrv = 0;  // size of raw storage, in number of `Header`s [!]
    uint32_t m_used = 0;  // size of used storage, in number of `Header`s [!]

  public:
    constexpr
    AVMC_Queue()
    noexcept
      { }

    AVMC_Queue(AVMC_Queue&& other)
    noexcept
      { this->swap(other);  }

    AVMC_Queue&
    operator=(AVMC_Queue&& other)
    noexcept
      { return this->swap(other);  }

    ~AVMC_Queue()
      {
        if(this->m_used)
          this->do_destroy_nodes();

        if(this->m_bptr)
          ::operator delete(this->m_bptr);

#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this), 0xCA, sizeof(*this));
#endif
      }

  private:
    void
    do_destroy_nodes()
    noexcept;

    void
    do_reallocate(uint32_t nadd);

    // Reserve storage for the next node.
    inline
    Header*
    do_reserve_one(Uparam up, const opt<Symbols>& syms_opt, size_t nbytes);

    // Append a new node to the end. `nbytes` is the size of `sp` to initialize.
    // If `src_opt` is specified, it should point to the buffer containing data to copy.
    // Otherwise, `sp` is filled with zeroes.
    AVMC_Queue&
    do_append_trivial(Executor* executor, Uparam up, opt<Symbols>&& syms_opt,
                      size_t nbytes, const void* src_opt);

    // Append a new node to the end. `nbytes` is the size of `sp` to initialize.
    // If `ctor_opt` is specified, it is called to initialize `sp`.
    // Otherwise, `sp` is filled with zeroes.
    AVMC_Queue&
    do_append_nontrivial(const details_avmc_queue::Vtable* vtbl, Uparam up, opt<Symbols>&& syms_opt,
                         size_t nbytes, details_avmc_queue::Constructor* ctor_opt, intptr_t ctor_arg);

    // Append a trivial or non-trivial node basing on trivialness of the argument.
    template<Executor execT, Enumerator* qvenumT, typename XSparamT>
    AVMC_Queue&
    do_append_chk(Uparam up, opt<Symbols>&& syms_opt, XSparamT&& xsp)
      {
        using Sparam = typename ::std::decay<XSparamT>::type;

        if(::std::is_trivial<Sparam>::value)
          return this->do_append_trivial(
                     execT, up, ::std::move(syms_opt),
                     sizeof(Sparam), ::std::addressof(xsp));

        return this->do_append_nontrivial(
                     details_avmc_queue::get_vtable<execT, qvenumT, Sparam>(), up,
                     ::std::move(syms_opt), sizeof(Sparam),
                     +[](Uparam, void* sp, intptr_t ctor_arg)
                       { ::rocket::construct_at(static_cast<Sparam*>(sp),
                               static_cast<XSparamT&&>(*(reinterpret_cast<Sparam*>(ctor_arg))));  },
                     reinterpret_cast<intptr_t>(static_cast<const void*>(
                                                   ::std::addressof(xsp))));
      }

  public:
    bool
    empty()
    const noexcept
      { return this->m_used == 0;  }

    AVMC_Queue&
    clear()
    noexcept
      {
        if(this->m_used)
          this->do_destroy_nodes();

        // Clean invalid data up.
        this->m_used = 0;
        return *this;
      }

    AVMC_Queue&
    shrink_to_fit()
      {
        if(this->m_used != this->m_rsrv)
          this->do_reallocate(0);
        return *this;
      }

    AVMC_Queue&
    swap(AVMC_Queue& other)
    noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_rsrv, other.m_rsrv);
        ::std::swap(this->m_used, other.m_used);
        return *this;
      }

    // Append a trivial node. This allows you to bind an arbitrary function.
    // If `sp_opt` is a null pointer, `nbytes` zero bytes are allocated.
    // Call `append()` if the parameter is non-trivial.
    AVMC_Queue&
    append_trivial(Executor& executor, Uparam up = { })
      { return this->do_append_trivial(executor, up, nullopt, 0, nullptr);  }

    AVMC_Queue&
    append_trivial(Executor& executor, Symbols syms, Uparam up = { })
      { return this->do_append_trivial(executor, up, ::std::move(syms), 0, nullptr);  }

    AVMC_Queue&
    append_trivial(Executor& executor, const void* sp_opt, size_t nbytes)
      { return this->do_append_trivial(executor, Uparam(), nullopt, nbytes, sp_opt);  }

    AVMC_Queue&
    append_trivial(Executor& executor, Symbols syms, const void* sp_opt, size_t nbytes)
      { return this->do_append_trivial(executor, Uparam(), ::std::move(syms), nbytes, sp_opt);  }

    AVMC_Queue&
    append_trivial(Executor& executor, Uparam up, const void* sp_opt, size_t nbytes)
      { return this->do_append_trivial(executor, up, nullopt, nbytes, sp_opt);  }

    AVMC_Queue&
    append_trivial(Executor& executor, Symbols syms, Uparam up, const void* sp_opt, size_t nbytes)
      { return this->do_append_trivial(executor, up, ::std::move(syms), nbytes, sp_opt);  }

    // Append a node with type-generic semantics.
    // Both trivial and non-trivial parameter types are supported.
    // However, as this may result in a virtual call, the executor function has to be specified as
    // a template argument.
    template<Executor execT, Enumerator* = nullptr>
    AVMC_Queue&
    append(Uparam up = { })
      { return this->do_append_trivial(execT, up, nullopt, 0, nullptr);  }

    template<Executor execT, Enumerator* = nullptr>
    AVMC_Queue&
    append(Symbols syms, Uparam up = { })
      { return this->do_append_trivial(execT, up, ::std::move(syms), 0, nullptr);  }

    template<Executor execT, Enumerator* qvenumT, typename XSparamT,
    ROCKET_DISABLE_IF(::std::is_convertible<XSparamT&&, Uparam>::value),
    ROCKET_DISABLE_IF(::std::is_convertible<XSparamT&&, Symbols>::value)>
    AVMC_Queue&
    append(XSparamT&& xsp)
      { return this->do_append_chk<execT, qvenumT>(Uparam(), nullopt, ::std::forward<XSparamT>(xsp));  }

    template<Executor execT, Enumerator* qvenumT, typename XSparamT,
    ROCKET_DISABLE_IF(::std::is_convertible<XSparamT&&, Uparam>::value),
    ROCKET_DISABLE_IF(::std::is_convertible<XSparamT&&, Symbols>::value)>
    AVMC_Queue&
    append(Symbols syms, XSparamT&& xsp)
      { return this->do_append_chk<execT, qvenumT>(Uparam(), ::std::move(syms), ::std::forward<XSparamT>(xsp));  }

    template<Executor execT, Enumerator* qvenumT, typename XSparamT>
    AVMC_Queue&
    append(Uparam up, XSparamT&& xsp)
      { return this->do_append_chk<execT, qvenumT>(up, nullopt, ::std::forward<XSparamT>(xsp));  }

    template<Executor execT, Enumerator* qvenumT, typename XSparamT>
    AVMC_Queue&
    append(Symbols syms, Uparam up, XSparamT&& xsp)
      { return this->do_append_chk<execT, qvenumT>(up, ::std::move(syms), ::std::forward<XSparamT>(xsp));  }

    // These are interfaces called by the runtime.
    AIR_Status
    execute(Executive_Context& ctx)
    const;

    Variable_Callback&
    enumerate_variables(Variable_Callback& callback)
    const;
  };

inline
void
swap(AVMC_Queue& lhs, AVMC_Queue& rhs)
noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
