// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_HPP_
#define ASTERIA_LLDS_AVMC_QUEUE_HPP_

#include "../fwd.hpp"
#include "../runtime/enums.hpp"
#include "../source_location.hpp"

namespace asteria {

class AVMC_Queue
  {
  public:
    // This union can be used to encapsulate trivial information in solidified nodes.
    // At most 48 btis can be stored here. You may make appropriate use of them.
    // Fields of each struct here share a unique prefix. This helps you ensure that
    // you don't mix access to fields of different structs at the same time.
    union Uparam
      {
        struct {
          uint16_t x_DO_NOT_USE_;
          uint16_t x16;
          uint32_t x32;
        };
        struct {
          uint16_t y_DO_NOT_USE_;
          uint8_t y8s[2];
          uint32_t y32;
        };
        struct {
          uint16_t z_DO_NOT_USE_;
          uint16_t z16s[3];
        };
        struct {
          uint16_t v_DO_NOT_USE_;
          uint8_t v8s[6];
        };
      };
    static_assert(sizeof(Uparam) == sizeof(uint64_t));

    // Symbols are optional. If no symbol is given, no backtrace frame is appended.
    // The source location is used to generate backtrace frames.
    struct Symbols
      {
        Source_Location sloc;
      };
    static_assert(::std::is_nothrow_move_constructible<Symbols>::value);

    // These are prototypes for callbacks.
    using Constructor  = void (Uparam uparam, void* sparam, intptr_t ctor_arg);
    using Move_Ctor    = void (Uparam uparam, void* sparam, void* sp_old);
    using Destructor   = void (Uparam uparam, void* sparam);
    using Executor     = AIR_Status (Executive_Context& ctx, Uparam uparam, const void* sparam);
    using Enumerator   = Variable_Callback& (Variable_Callback& callback, Uparam uparam, const void* sparam);

  private:
    struct Header;

    struct Vtable
      {
        Move_Ctor* mvctor_opt;  // if null then bitwise copy is performed
        Destructor* dtor_opt;  // if null then no cleanup is performed
        Executor* executor;  // not nullable [!]
        Enumerator* venum_opt;  // if null then no variables shall exist
      };

  private:
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
    do_reserve_one(Uparam uparam, const opt<Symbols>& syms_opt, size_t nbytes);

    // This function returns a vtable struct that is allocated statically.
    template<Executor execT, Enumerator* qvenumT, typename SparamT>
    static
    const Vtable*
    do_get_vtable()
    noexcept
      {
        static_assert(::std::is_object<SparamT>::value);
        static_assert(::std::is_same<SparamT, typename ::std::decay<SparamT>::type>::value);
        static_assert(::std::is_nothrow_move_constructible<SparamT>::value);

        // The vtable must have static storage duration.
        // As it is defined `constexpr` here, we need 'real' function pointers.
        // Those converted from non-capturing lambdas are not an option.
        struct Sfn
          {
            static
            void
            move_construct(Uparam, void* sparam, void* sp_old)
            noexcept
              { ::rocket::construct_at((SparamT*)sparam, ::std::move(*(SparamT*)sp_old));  }

            static
            void
            destroy(Uparam, void* sparam)
            noexcept
              { ::rocket::destroy_at((SparamT*)sparam);  }
          };

        // Trivial operations can be optimized.
        static constexpr Vtable s_vtbl[1] =
          {{
            ::std::is_trivially_move_constructible<SparamT>::value ? nullptr : Sfn::move_construct,
            ::std::is_trivially_destructible<SparamT>::value ? nullptr : Sfn::destroy,
            execT,
            qvenumT,
          }};
        return s_vtbl;
      }

    // Append a new node to the end. `nbytes` is the size of `sparam` to initialize.
    // If `src_opt` is specified, it should point to the buffer containing data to copy.
    // Otherwise, `sparam` is filled with zeroes.
    void
    do_append_trivial(Executor* exec, Uparam uparam, opt<Symbols>&& syms_opt, size_t nbytes,
                      const void* src_opt);

    // Append a new node to the end. `nbytes` is the size of `sparam` to initialize.
    // If `ctor_opt` is specified, it is called to initialize `sparam`.
    // Otherwise, `sparam` is filled with zeroes.
    void
    do_append_nontrivial(const Vtable* vtbl, Uparam uparam, opt<Symbols>&& syms_opt, size_t nbytes,
                         Constructor* ctor_opt, intptr_t ctor_arg);

    // Append a trivial or non-trivial node basing on trivialness of the argument.
    template<Executor execT, Enumerator* qvenumT, typename XSparamT>
    void
    do_append_chk(Uparam uparam, opt<Symbols>&& syms_opt, XSparamT&& xsparam)
      {
        using Sparam = typename ::std::decay<XSparamT>::type;

        if(::std::is_trivial<Sparam>::value)
          this->do_append_trivial(
              execT, uparam, ::std::move(syms_opt), sizeof(Sparam), ::std::addressof(xsparam));
        else
          this->do_append_nontrivial(
              this->do_get_vtable<execT, qvenumT, Sparam>(), uparam, ::std::move(syms_opt), sizeof(Sparam),
              +[](Uparam /*uparam*/, void* sparam, intptr_t ctor_arg)
                { ::rocket::construct_at((Sparam*)sparam, (XSparamT&&)*(Sparam*)ctor_arg);  },
              (intptr_t)(const void*)::std::addressof(xsparam));
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
        if(this->m_used == this->m_rsrv)
          return *this;

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
    // If `sparam_opt` is a null pointer, `nbytes` zero bytes are allocated.
    // Call `append()` if the parameter is non-trivial.
    AVMC_Queue&
    append_trivial(Executor& exec, Uparam uparam = { })
      {
        this->do_append_trivial(exec, uparam, nullopt, 0, nullptr);
        return *this;
      }

    AVMC_Queue&
    append_trivial(Executor& exec, Symbols syms, Uparam uparam = { })
      {
        this->do_append_trivial(exec, uparam, ::std::move(syms), 0, nullptr);
        return *this;
      }

    AVMC_Queue&
    append_trivial(Executor& exec, const void* sparam_opt, size_t nbytes)
      {
        this->do_append_trivial(exec, Uparam(), nullopt, nbytes, sparam_opt);
        return *this;
      }

    AVMC_Queue&
    append_trivial(Executor& exec, Symbols syms, const void* sparam_opt, size_t nbytes)
      {
        this->do_append_trivial(exec, Uparam(), ::std::move(syms), nbytes, sparam_opt);
        return *this;
      }

    AVMC_Queue&
    append_trivial(Executor& exec, Uparam uparam, const void* sparam_opt, size_t nbytes)
      {
        this->do_append_trivial(exec, uparam, nullopt, nbytes, sparam_opt);
        return *this;
      }

    AVMC_Queue&
    append_trivial(Executor& exec, Symbols syms, Uparam uparam, const void* sparam_opt, size_t nbytes)
      {
        this->do_append_trivial(exec, uparam, ::std::move(syms), nbytes, sparam_opt);
        return *this;
      }

    // Append a node with type-generic semantics.
    // Both trivial and non-trivial parameter types are supported.
    // However, as this may result in a virtual call, the executor function has to be specified as
    // a template argument.
    template<Executor execT, Enumerator* = nullptr>
    AVMC_Queue&
    append(Uparam uparam = { })
      {
        this->do_append_trivial(execT, uparam, nullopt, 0, nullptr);
        return *this;
      }

    template<Executor execT, Enumerator* = nullptr>
    AVMC_Queue&
    append(Symbols syms, Uparam uparam = { })
      {
        this->do_append_trivial(execT, uparam, ::std::move(syms), 0, nullptr);
        return *this;
      }

    template<Executor execT, Enumerator* qvenumT, typename XSparamT,
    ROCKET_DISABLE_IF(::rocket::disjunction<::std::is_convertible<XSparamT&&, Uparam>,
                                            ::std::is_convertible<XSparamT&&, Symbols>>::value)>
    AVMC_Queue&
    append(XSparamT&& xsparam)
      {
        this->do_append_chk<execT, qvenumT>(Uparam(), nullopt, ::std::forward<XSparamT>(xsparam));
        return *this;
      }

    template<Executor execT, Enumerator* qvenumT, typename XSparamT,
    ROCKET_DISABLE_IF(::rocket::disjunction<::std::is_convertible<XSparamT&&, Uparam>,
                                            ::std::is_convertible<XSparamT&&, Symbols>>::value)>
    AVMC_Queue&
    append(Symbols syms, XSparamT&& xsparam)
      {
        this->do_append_chk<execT, qvenumT>(Uparam(), ::std::move(syms), ::std::forward<XSparamT>(xsparam));
        return *this;
      }

    template<Executor execT, Enumerator* qvenumT, typename XSparamT>
    AVMC_Queue&
    append(Uparam uparam, XSparamT&& xsparam)
      {
        this->do_append_chk<execT, qvenumT>(uparam, nullopt, ::std::forward<XSparamT>(xsparam));
        return *this;
      }

    template<Executor execT, Enumerator* qvenumT, typename XSparamT>
    AVMC_Queue&
    append(Symbols syms, Uparam uparam, XSparamT&& xsparam)
      {
        this->do_append_chk<execT, qvenumT>(uparam, ::std::move(syms), ::std::forward<XSparamT>(xsparam));
        return *this;
      }

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
