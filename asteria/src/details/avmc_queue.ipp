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
    max_align_t stor[0];
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
      const Vtable* vtbl;        // active if `has_vtbl`
      Executor* executor;        // active otherwise
    };

    union {
      Sparam_syms sp_syms[0];    // active if `has_syms`
      max_align_t sp_stor[0];    // active otherwise
    };

    // Begin fancy optimization for space!
    const Uparam&
    uparam()
    const noexcept
      { return this->up_stor;  }

    Uparam&
    uparam()
    noexcept
      { return this->up_stor;  }

    const void*
    sparam()
    const noexcept
      { return this->has_syms ? this->sp_syms->stor : this->sp_stor;  }

    void*
    sparam()
    noexcept
      { return this->has_syms ? this->sp_syms->stor : this->sp_stor;  }

    uint32_t
    total_size_in_headers()
    const noexcept
      { return UINT32_C(1) + this->has_syms + this->nphdrs;  }

    // Provide general accesstors.
    void
    relocate(Header&& other)
    noexcept
      {
        // Move user-defined data. The symbols are trivially relocatable.
        if(this->has_vtbl && this->vtbl->reloc_opt)
          this->vtbl->reloc_opt(this->uparam(), this->sparam(), other.sparam());
      }

    void
    destroy()
    noexcept
      {
        // Destroy user-defined data and symbols.
        if(this->has_vtbl && this->vtbl->dtor_opt)
          this->vtbl->dtor_opt(this->uparam(), this->sparam());

        if(this->has_syms)
          delete this->sp_syms->syms_opt;
      }

    AIR_Status
    execute(Executive_Context& ctx)
    const
      {
        if(this->has_vtbl)
          return this->vtbl->executor(ctx, this->uparam(), this->sparam());
        else
          return this->executor(ctx, this->uparam(), this->sparam());
      }

    void
    enumerate_variables(Variable_Callback& callback)
    const
      {
        if(this->has_vtbl && this->vtbl->venum_opt)
          this->vtbl->venum_opt(callback, this->uparam(), this->sparam());
      }

    void
    adopt_symbols(uptr<Symbols>&& syms_opt)
    noexcept
      {
        ROCKET_ASSERT(this->has_syms);
        this->sp_syms->syms_opt = syms_opt.release();
      }

    ASTERIA_INCOMPLET(Runtime_Error)
    void
    push_symbols(Runtime_Error& except)
    const
      {
        if(this->has_syms && this->sp_syms->syms_opt)
          except.push_frame_plain(this->sp_syms->syms_opt->sloc, ::rocket::sref(""));
      }
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
