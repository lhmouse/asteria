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
using Constructor  = void (Uparam uparam, void* sparam, intptr_t ctor_arg);
using Relocator    = void (Uparam uparam, void* sparam, void* sp_old);
using Destructor   = void (Uparam uparam, void* sparam);
using Executor     = AIR_Status (Executive_Context& ctx, Uparam uparam, const void* sparam);
using Enumerator   = Variable_Callback& (Variable_Callback& callback, Uparam uparam, const void* sparam);

struct Vtable
  {
    Relocator* reloc_opt;  // if null then bitwise copy is performed
    Destructor* dtor_opt;  // if null then no cleanup is performed
    Executor* executor;  // not nullable [!]
    Enumerator* venum_opt;  // if null then no variables shall exist
  };

struct Sparam_syms
  {
    const Symbols* syms_opt;
    void* reserved;
    max_align_t aligned[0];
  };

// This is the header of each variable-length object that is stored in an AVMC queue.
// User-defined data may immediate follow this struct, so the size of this struct has
// to be a multiple of `alignof(max_align_t)`.
struct Header
  {
    union {
      struct {
        uint8_t nphdrs;         // size of `sparam`, in number of `Header`s [!]
        uint8_t has_vtbl : 1;   // vtable exists?
        uint8_t has_syms : 1;   // symbols exist?
      };
      Uparam up_stor;
    };

    union {
      const Vtable* vtable;      // active if `has_vtbl`
      Executor* exec;            // active otherwise
    };

    union {
      Sparam_syms sp_syms[0];    // active if `has_syms`
      max_align_t sp_stor[0];   // active otherwise
    };

    // The Executor function and Uparam struct always exist.
    Executor*
    executor()
    const noexcept
      { return this->has_vtbl ? this->vtable->executor : this->exec;  }

    Uparam
    uparam()
    const noexcept
      { return this->up_stor;  }

    // These functions obtain function pointers from the Vtable, if any.
    Relocator*
    reloc_opt()
    const noexcept
      { return this->has_vtbl ? this->vtable->reloc_opt : nullptr;  }

    Destructor*
    dtor_opt()
    const noexcept
      { return this->has_vtbl ? this->vtable->dtor_opt : nullptr;  }

    Enumerator*
    venum_opt()
    const noexcept
      { return this->has_vtbl ? this->vtable->venum_opt : nullptr;  }

    // These functions access subsequential user-defined data.
    const Symbols*
    syms_opt()
    const noexcept
      { return this->has_syms ? this->sp_syms->syms_opt : nullptr;  }

    const void*
    sparam()
    const noexcept
      { return this->has_syms ? this->sp_syms->aligned : this->sp_stor;  }

    void*
    sparam()
    noexcept
      { return this->has_syms ? this->sp_syms->aligned : this->sp_stor;  }

    uint32_t
    total_size_in_headers()
    const noexcept
      { return UINT32_C(1) + this->has_syms + this->nphdrs;  }
  };

static_assert(sizeof(Header) == sizeof(Sparam_syms), "");
static_assert(sizeof(Header) % alignof(max_align_t) == 0, "");
static_assert(alignof(Header) == alignof(max_align_t), "");

template<typename SparamT, typename = void>
struct Default_reloc_opt
  {
    static
    void
    value(Uparam, void* sparam, void* sp_old)
    noexcept
      {
        ::rocket::construct_at((SparamT*)sparam, ::std::move(*(SparamT*)sp_old));
        ::rocket::destroy_at((SparamT*)sp_old);
      }
  };

template<typename SparamT>
struct Default_reloc_opt<SparamT, typename ::std::enable_if<
                   ::std::is_trivially_copyable<SparamT>::value
                   >::type>
  {
    static constexpr Relocator* value = nullptr;
  };

template<typename SparamT, typename = void>
struct Default_dtor_opt
  {
    static
    void
    value(Uparam, void* sparam)
    noexcept
      {
        ::rocket::destroy_at((SparamT*)sparam);
      }
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

    static constexpr Vtable s_vtbl[1] = {{ Default_reloc_opt<SparamT>::value,
                                           Default_dtor_opt<SparamT>::value,
                                           execT, qvenumT }};
    return s_vtbl;
  }

}  // namespace details_avmc_queue
}  // namespace asteria
