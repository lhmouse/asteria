// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

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

struct Metadata;
struct Header;

// These are prototypes for callbacks.
using Constructor  = void (Header* head, size_t size, intptr_t arg);
using Relocator    = void (Header* head, Header* from);
using Destructor   = void (Header* head);
using Executor     = AIR_Status (Executive_Context& ctx, const Header* head);
using Enumerator   = Variable_Callback& (Variable_Callback& callback, const Header* head);

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

    max_align_t align[0];
    char sparam[0];
  };

template<typename SparamT, typename = void>
struct default_reloc
  {
    static
    void
    func_opt(Header* head, Header* from)
      noexcept
      {
        auto target = reinterpret_cast<SparamT*>(head->sparam);
        auto source = reinterpret_cast<SparamT*>(from->sparam);
        ::rocket::construct_at(target, ::std::move(*source));
        ::rocket::destroy_at(source);
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
    func_opt(Header* head)
      noexcept
      {
        auto target = reinterpret_cast<SparamT*>(head->sparam);
        ::rocket::destroy_at(target);
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
    func(Header* head, size_t /*size*/, intptr_t arg)
      {
        auto target = reinterpret_cast<SparamT*>(head->sparam);
        auto source = reinterpret_cast<SparamT*>(arg);
        ::rocket::construct_at(target, static_cast<XSparamT&&>(*source));
      }
  };

struct memcpy_ctor
  {
    static
    void
    func(Header* head, size_t size, intptr_t arg)
      {
        ::std::memcpy(head->sparam, reinterpret_cast<char*>(arg), size);
      }
  };

}  // namespace details_avmc_queue
}  // namespace asteria
