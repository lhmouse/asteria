// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVM_ROD_
#define ASTERIA_LLDS_AVM_ROD_

#include "../fwd.hpp"
#include "../source_location.hpp"
namespace asteria {

class AVM_Rod
  {
  public:
    struct Metadata;
    struct Header;

    // These are prototypes for callbacks.
    using Executor     = void (Executive_Context& ctx, const Header* head);
    using Constructor  = void (Header* head, void* ctor_arg);
    using Destructor   = void (Header* head);
    using Collector    = void (Variable_HashMap& staged, Variable_HashMap& temp, const Header* head);

    // This union can be used to encapsulate trivial information in solidified
    // nodes. Each field is assigned a unique suffix. At most 48 bits can be
    // stored here. You may make appropriate use of them.
    union Uparam
      {
        struct {
          char _xb1[2];
          bool b0, b1, b2, b3, b4, b5;
        };
        struct {
          char _xc1[2];
          char c0, c1, c2, c3, c4, c5;
        };
        struct {
          char _xi1[2];
          int8_t i0, i1, i2, i3, i4, i5;
        };
        struct {
          char _xu1[2];
          uint8_t u0, u1, u2, u3, u4, u5;
        };
        struct {
          char _xi2[2];
          int16_t i01, i23, i45;
        };
        struct {
          char _xu2[2];
          uint16_t u01, u23, u45;
        };
        struct {
          char _xi4[4];
          int32_t i2345;
        };
        struct {
          char _xu4[4];
          uint32_t u2345;
        };
      };

    struct Metadata
      {
        Executor* exec;
        Destructor* dtor_opt;
        Collector* coll_opt;
        opt<Source_Location> sloc_opt;
      };

    // This is the header of each variable-length element that is stored in an
    // AVM rod. User-defined data (the `sparam`) may follow this struct, so the
    // size of this struct has to be a multiple of `alignof(max_align_t)`.
    struct Header
      {
        union {
          Uparam uparam;
          struct {
            uint8_t nheaders;  // size of `sparam`, in number of headers [!]
            uint8_t has_pv_meta : 1;
            uint8_t reserved : 7;
          };
        };

        union {
          Executor* pv_exec;  // executor function
          Metadata* pv_meta;
        };

        alignas(max_align_t) char sparam[];
      };

  private:
    Header* m_bptr = nullptr;
    uint32_t m_einit = 0;
    uint32_t m_estor = 0;

  public:
    constexpr
    AVM_Rod()
      noexcept = default;

    AVM_Rod(AVM_Rod&& other)
      noexcept
      {
        this->swap(other);
      }

    AVM_Rod&
    operator=(AVM_Rod&& other)
      & noexcept
      {
        this->swap(other);
        return *this;
      }

    AVM_Rod&
    swap(AVM_Rod& other)
      noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_einit, other.m_einit);
        ::std::swap(this->m_estor, other.m_estor);
        return *this;
      }

  private:
    void
    do_reallocate(uint32_t estor);

    void
    do_deallocate()
      noexcept;

  public:
    ~AVM_Rod()
      {
        if(this->m_bptr)
          this->do_deallocate();
      }

    bool
    empty()
      const noexcept
      { return this->m_einit == 0;  }

    void
    clear()
      noexcept;

    // Append a new node to the end. `size` is the size of `sparam` to initialize.
    // If `ctor_opt` is specified, it is called to initialize `sparam`. Otherwise,
    // `sparam` is filled with zeroes. If `sloc_opt` is specified, it denotes the
    // symbols for backtracing, which are copied and stored by these functions and
    // need not be persistent.
    Header*
    push_function(Executor* exec, Uparam uparam, size_t sparam_size, Constructor* ctor_opt,
                  void* ctor_arg, Destructor* dtor_opt, Collector* coll_opt,
                  const Source_Location* sloc_opt);

    // Marks this rod ready for execution. No nodes may be appended hereafter.
    // This function serves as an optimization hint.
    void
    finalize();

    // These are internal interfaces that are called by the runtime.
    void
    execute(Executive_Context& ctx)
      const;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp)
      const;
  };

inline
void
swap(AVM_Rod& lhs, AVM_Rod& rhs)
  noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
