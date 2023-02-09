// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_
#  error Please include <asteria/llds/avmc_queue.hpp> instead.
#endif
namespace asteria {
namespace details_avmc_queue {

// This union can be used to encapsulate trivial information in solidified
// nodes. Each field is assigned a unique suffix. At most 48 bits can be
// stored here. You may make appropriate use of them.
union Uparam
  {
    struct {
      uint16_t _do_not_use_0_;
      uint16_t u16;
      uint32_t u32;
    };

    struct {
      uint16_t _do_not_use_1_;
      uint8_t u8v[6];
    };

    struct {
      uint16_t _do_not_use_2_;
      uint16_t u16p[3];
    };
  };

struct Metadata;
struct Header;

// These are prototypes for callbacks.
using Constructor  = void (Header* head, intptr_t arg);
using Relocator    = void (Header* head, Header* from);
using Destructor   = void (Header* head);
using Executor     = AIR_Status (Executive_Context& ctx, const Header* head);
using Var_Getter   = void (Variable_HashMap& staged, Variable_HashMap& temp,
                           const Header* head);

struct Metadata
  {
    // Version 1
    Relocator* reloc_opt;  // if null then bitwise copy is performed
    Destructor* dtor_opt;  // if null then no cleanup is performed
    Var_Getter* vget_opt;  // if null then no variable shall exist
    Executor* exec;        // executor function, must not be null

    // Version 2
    Source_Location syms;  // symbols
  };

// This is the header of each variable-length element that is stored in an AVMC
// queue. User-defined data (the `sparam`) may follow this struct, so the size
// of this struct has to be a multiple of `alignof(max_align_t)`.
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

    alignas(max_align_t) char sparam[];
  };

template<typename SparamT>
inline
void
do_nontrivial_reloc(Header* head, Header* from)
  {
    auto ptr = reinterpret_cast<SparamT*>(head->sparam);
    auto src = reinterpret_cast<SparamT*>(from->sparam);
    ::rocket::construct(ptr, ::std::move(*src));
    ::rocket::destroy(src);
  }

template<typename SparamT>
inline
void
do_nontrivial_dtor(Header* head)
  {
    auto ptr = reinterpret_cast<SparamT*>(head->sparam);
    ::rocket::destroy(ptr);
  }

template<typename SparamT>
inline
void
do_call_get_variables(Variable_HashMap& staged, Variable_HashMap& temp,
                      const Header* head)
  {
    auto ptr = reinterpret_cast<const SparamT*>(head->sparam);
    ptr->get_variables(staged, temp);
  }

template<typename SparamT, typename = void>
struct select_get_variables
  {
    constexpr operator
    Var_Getter*() const noexcept
      { return nullptr;  }
  };

template<typename SparamT>
struct select_get_variables<SparamT,
    ROCKET_VOID_DECLTYPE(
      ::std::declval<const SparamT&>().get_variables(
          ::std::declval<Variable_HashMap&>(),  // staged
          ::std::declval<Variable_HashMap&>()   // temp
        ))>
  {
    constexpr operator
    Var_Getter*() const noexcept
      { return do_call_get_variables<SparamT>;  }
  };

template<typename SparamT>
struct Sparam_traits
  {
    static constexpr Relocator* reloc_opt =
        ::std::is_trivially_copyable<SparamT>::value
            ? nullptr
            : do_nontrivial_reloc<SparamT>;

    static constexpr Destructor* dtor_opt =
        ::std::is_trivially_destructible<SparamT>::value
            ? nullptr
            : do_nontrivial_dtor<SparamT>;

    static constexpr Var_Getter* vget_opt =
        select_get_variables<SparamT>();
  };

template<typename XSparamT>
inline
void
do_forward_ctor(Header* head, intptr_t arg)
  {
    using Sparam = typename ::std::decay<XSparamT>::type;
    auto ptr = reinterpret_cast<Sparam*>(head->sparam);
    auto src = reinterpret_cast<Sparam*>(arg);
    ::rocket::construct(ptr, static_cast<XSparamT&&>(*src));
  }

}  // namespace details_avmc_queue
}  // namespace asteria
