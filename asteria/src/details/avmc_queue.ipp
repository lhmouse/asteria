// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_HPP_
#  error Please include <asteria/llds/avmc_queue.hpp> instead.
#endif

namespace asteria {
namespace details_avmc_queue {

// This union can be used to encapsulate trivial information in solidified nodes.
// At most 48 bits can be stored here. You may make appropriate use of them.
union Uparam
  {
    struct {
      uint16_t _do_not_use_0_;
      uint16_t s16;
      uint32_t s32;
    };

    struct {
      uint16_t _do_not_use_1_;
      uint8_t p8[6];
    };

    struct {
      uint16_t _do_not_use_2_;
      uint16_t p16[3];
    };
  };

// These are prototypes for callbacks.
using Constructor  = void (Uparam uparam, void* sparam, size_t size, intptr_t arg);
using Relocator    = void (Uparam uparam, void* sparam, void* source);
using Destructor   = void (Uparam uparam, void* sparam);
using Executor     = AIR_Status (Executive_Context& ctx, Uparam uparam, const void* sparam);
using Enumerator   = Variable_Callback& (Variable_Callback& callback, Uparam uparam, const void* sparam);

struct Metadata
  {
    // Version 1
    Relocator* reloc_opt;  // if null then bitwise copy is performed
    Destructor* dtor_opt;  // if null then no cleanup is performed
    Enumerator* enum_opt;  // if null then no variable shall exist
    Executor* exec;        // executor function, must not be null

    // Version 2
    Source_Location syms;  // symbols
  };

// This is the header of each variable-length element that is stored in an AVMC
// queue. User-defined data may immediate follow this struct, so the size of
// this struct has to be a multiple of `alignof(max_align_t)`.
struct Header
  {
    union {
      struct {
        uint8_t nheaders;  // size of `sparam`, in number of headers [!]
        uint8_t meta_ver;  // version of `Metadata`; `pv_meta` active
      };
      Uparam uparam;
    };

    union {
      Executor* pv_exec;  // executor function
      Metadata* pv_meta;
    };

    max_align_t sparam[0];
  };

template<typename SparamT, typename = void>
struct default_reloc
  {
    static
    void
    func_opt(Uparam, void* sparam, void* source)
      noexcept
      {
        ::rocket::construct_at(static_cast<SparamT*>(sparam),
                        ::std::move(*static_cast<SparamT*>(source)));
        ::rocket::destroy_at(static_cast<SparamT*>(source));
      }
  };

template<typename SparamT>
struct default_reloc<SparamT, typename ::std::enable_if<
           ::std::is_trivially_copyable<SparamT>::value>::type>
  {
    static constexpr Relocator* func_opt = nullptr;
  };

template<typename SparamT, typename = void>
struct default_dtor
  {
    static
    void
    func_opt(Uparam, void* sparam)
      noexcept
      {
        ::rocket::destroy_at(static_cast<SparamT*>(sparam));
      }
  };

template<typename SparamT>
struct default_dtor<SparamT, typename ::std::enable_if<
           ::std::is_trivially_destructible<SparamT>::value>::type>
  {
    static constexpr Destructor* func_opt = nullptr;
  };

template<typename SparamT, typename XSparamT>
struct forward_ctor
  {
    static
    void
    func(Uparam, void* sparam, size_t, intptr_t arg)
      {
        ::rocket::construct_at(static_cast<SparamT*>(sparam),
             static_cast<XSparamT&&>(*(reinterpret_cast<
                 typename ::std::remove_reference<XSparamT>::type*>(arg))));
      }
  };

struct memcpy_ctor
  {
    static
    void
    func(Uparam, void* sparam, size_t size, intptr_t arg)
      {
        ::std::memcpy(sparam, reinterpret_cast<const void*>(arg), size);
      }
  };

}  // namespace details_avmc_queue
}  // namespace asteria
