// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_HPP_
#  error Please include <asteria/llds/avmc_queue.hpp> instead.
#endif

namespace asteria {
namespace details_avmc_queue {

// This union can be used to encapsulate trivial information in solidified nodes.
// At most 48 bits can be stored here. You may make appropriate use of them.
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

// Symbols are optional. If no symbol is given, no backtrace frame is appended.
// The source location is used to generate backtrace frames.
struct Symbols
  {
    Source_Location sloc;
  };

// These are prototypes for callbacks.
using Constructor       = void (Uparam uparam, void* sparam, intptr_t ctor_arg);
using Move_Constructor  = void (Uparam uparam, void* sparam, void* sp_old);
using Destructor        = void (Uparam uparam, void* sparam);
using Executor          = AIR_Status (Executive_Context& ctx, Uparam uparam, const void* sparam);
using Enumerator        = Variable_Callback& (Variable_Callback& callback, Uparam uparam, const void* sparam);

struct Vtable
  {
    Move_Constructor* mvct_opt;  // if null then bitwise copy is performed
    Destructor* dtor_opt;  // if null then no cleanup is performed
    Executor* executor;  // not nullable [!]
    Enumerator* venum_opt;  // if null then no variables shall exist
  };


struct Header
  {
    union {
      struct {
        uint8_t nphdrs;  // size of `sparam`, in number of `Header`s [!]
        uint8_t has_vtbl : 1;  // vtable exists?
        uint8_t has_syms : 1;  // symbols exist?
      };
      Uparam up_stor;
    };
    union {
      const Vtable* vtable;  // active if `has_vtbl`
      Executor* exec;  // active otherwise
    };
    max_align_t alignment[0];

    Move_Constructor*
    mvct_opt()
    const noexcept
      { return this->has_vtbl ? this->vtable->mvct_opt : nullptr;  }

    Destructor*
    dtor_opt()
    const noexcept
      { return this->has_vtbl ? this->vtable->dtor_opt : nullptr;  }

    Executor*
    executor()
    const noexcept
      { return this->has_vtbl ? this->vtable->executor : this->exec;  }

    Enumerator*
    venum_opt()
    const noexcept
      { return this->has_vtbl ? this->vtable->venum_opt : nullptr;  }

    constexpr
    const Uparam&
    uparam()
    const noexcept
      { return this->up_stor;  }

    constexpr
    void*
    skip(uint32_t offset)
    const noexcept
      { return const_cast<Header*>(this) + 1 + offset;  }

    Symbols*
    syms_opt()
    const noexcept
      { return this->has_syms ? static_cast<Symbols*>(this->skip(0)) : nullptr;  }

    constexpr
    uint32_t
    symbol_size_in_headers()
    const noexcept
      { return this->has_syms * uint32_t((sizeof(Symbols) - 1) / sizeof(Header) + 1);  }

    void*
    sparam()
    const noexcept
      { return this->skip(this->symbol_size_in_headers());  }

    constexpr
    uint32_t
    total_size_in_headers()
    const noexcept
      { return 1 + this->symbol_size_in_headers() + this->nphdrs;  }
  };

template<typename SparamT, typename = void>
struct Default_mvct_opt
  {
    static
    void
    value(Uparam, void* sparam, void* sp_old)
    noexcept
      { ::rocket::construct_at(static_cast<SparamT*>(sparam),
                               ::std::move(*(static_cast<SparamT*>(sp_old))));  }
  };

template<typename SparamT>
struct Default_mvct_opt<SparamT, typename ::std::enable_if<
                   ::std::is_trivially_move_constructible<SparamT>::value
                   >::type>
  {
    static constexpr Move_Constructor* value = nullptr;
  };

template<typename SparamT, typename = void>
struct Default_dtor_opt
  {
    static
    void
    value(Uparam, void* sparam)
    noexcept
      { ::rocket::destroy_at(static_cast<SparamT*>(sparam));  }
  };

template<typename SparamT>
struct Default_dtor_opt<SparamT, typename ::std::enable_if<
                   ::std::is_trivially_destructible<SparamT>::value
                   >::type>
  {
    static constexpr Destructor* value = nullptr;
  };

// This function returns a vtable struct that is allocated statically.
template<Executor execT, Enumerator* qvenumT, typename SparamT>
ROCKET_CONST_FUNCTION inline
const Vtable*
get_vtable()
noexcept
  {
    static_assert(!::std::is_array<SparamT>::value, "");
    static_assert(::std::is_object<SparamT>::value, "");
    static_assert(::std::is_nothrow_move_constructible<SparamT>::value, "");

    static constexpr Vtable s_vtbl[1] = {{ Default_mvct_opt<SparamT>::value,
                                           Default_dtor_opt<SparamT>::value,
                                           execT, qvenumT }};
    return s_vtbl;
  }

}  // namespace details_avmc_queue
}  // namespace asteria
