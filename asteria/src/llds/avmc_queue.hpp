// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_HPP_
#define ASTERIA_LLDS_AVMC_QUEUE_HPP_

#include "../fwd.hpp"

namespace Asteria {

class AVMC_Queue
  {
  public:
    union ParamU
      {
        struct {
          uint32_t x32;
          uint16_t x16;
        };
        struct {
          uint32_t y32;
          uint16_t y8s[2];
        };
        uint8_t u8s[6];
        uint16_t u16s[3];
      };

    // These are prototypes for callbacks.
    using Constructor  = void (ParamU paramu, void* params, intptr_t source);
    using Destructor   = void (ParamU paramu, void* params);
    using Executor     = AIR_Status (Executive_Context& ctx, ParamU paramu, const void* params);
    using Enumerator   = Variable_Callback& (Variable_Callback& callback, ParamU paramu, const void* params);

    // This specifies characteristics of the data contained.
    struct Vtable
      {
        Destructor* dtor;
        Executor* exec;
        Enumerator* vnum;
      };
    static_assert(std::is_trivial<Vtable>::value, "??");

  private:
    enum : size_t
      {
        nphdrs_max = 0x100,  // maximum value of `Header::nphdrs`
      };

    struct Header
      {
        uint16_t nphdrs : 8;  // size of `params`, in number of `sizeof(Header)` (!)
        uint16_t has_vtbl : 1;  // vtable exists?
        uint16_t paramu_x16;  // user-defined data (1)
        uint32_t paramu_x32;  // user-defined data (2)
        union {
          Executor* exec;  // active if `has_vtbl`
          const Vtable* vtbl;  // active otherwise
        };
        alignas(max_align_t) intptr_t params[];  // user-defined data (3)
      };

    struct Storage
      {
        Header* bptr;  // beginning of raw storage
        uint32_t nrsrv;  // size of raw storage, in number of `sizeof(Header)` (!)
        uint32_t nused;  // size of used storage, in number of `sizeof(Header)` (!)
      };
    Storage m_stor;

  public:
    constexpr AVMC_Queue() noexcept
      :
        m_stor()
      {
      }
    AVMC_Queue(AVMC_Queue&& other) noexcept
      :
        m_stor()
      {
        xswap(this->m_stor, other.m_stor);
      }
    AVMC_Queue& operator=(AVMC_Queue&& other) noexcept
      {
        xswap(this->m_stor, other.m_stor);
        return *this;
      }
    ~AVMC_Queue()
      {
        if(this->m_stor.bptr) {
          this->do_deallocate_storage();
        }
#ifdef ROCKET_DEBUG
        std::memset(std::addressof(this->m_stor), 0xE5, sizeof(m_stor));
#endif
      }

  private:
    void do_deallocate_storage() const;
    void do_execute_all_break(AIR_Status& status, Executive_Context& ctx) const;
    void do_enumerate_variables(Variable_Callback& callback) const;

    // Reserve storage for another node. `nbytes` is the size of `params` to reserve in bytes.
    // Note: All calls to this function must precede calls to `do_check_storage_for_params()`.
    void do_reserve_delta(size_t nbytes);
    // Allocate storage for all nodes that have been reserved so far, then checks whether there is enough room for a new node
    // with `params` whose size is `nbytes` in bytes. An exception is thrown in case of failure.
    Header* do_check_storage_for_params(size_t nbytes);
    // Append a new node to the end. `nbytes` is the size of `params` to initialize in bytes.
    // Note: The storage must have been reserved using `do_reserve_delta()`.
    void do_append_trivial(Executor* exec, ParamU paramu, size_t nbytes, const void* source);
    void do_append_nontrivial(ref_to<const Vtable> vtbl, ParamU paramu, size_t nbytes, Constructor* ctor, intptr_t source);

    template<Executor execT, nullptr_t, typename XnodeT>
        void do_dispatch_append(std::true_type, ParamU paramu, XnodeT&& xnode)
      {
        // The parameter type is trivial and no vtable is required.
        // Append a node with a trivial parameter.
        this->do_append_trivial(execT, paramu, sizeof(xnode), std::addressof(xnode));
      }
    template<Executor execT, Enumerator* enumT, typename XnodeT>
        void do_dispatch_append(std::false_type, ParamU paramu, XnodeT&& xnode)
      {
        // The vtable must have static storage duration. As it is defined `constexpr` here, we need 'real' function pointers.
        // Those converted from non-capturing lambdas are not an option.
        struct H
          {
            static void construct(ParamU /*paramu*/, void* params, intptr_t source)
              {
                // Construct the bound parameter using perfect forwarding.
                rocket::construct_at(static_cast<typename rocket::remove_cvref<XnodeT>::type*>(params),
                                     rocket::forward<XnodeT>(*(typename std::remove_reference<XnodeT>::type*)source));
              }
            static void destroy(ParamU /*paramu*/, void* params) noexcept
              {
                // Destroy the bound parameter.
                rocket::destroy_at(static_cast<typename rocket::remove_cvref<XnodeT>::type*>(params));
              }
          };
        static constexpr Vtable s_vtbl =
          {
            H::destroy, execT, enumT
          };
        // Append a node with a non-trivial parameter.
        this->do_append_nontrivial(rocket::ref(s_vtbl), paramu, sizeof(xnode), H::construct,
                                   reinterpret_cast<intptr_t>(std::addressof(xnode)));
      }

  public:
    bool empty() const noexcept
      {
        return this->m_stor.bptr == nullptr;
      }
    AVMC_Queue& clear() noexcept
      {
        if(this->m_stor.bptr) {
          this->do_deallocate_storage();
        }
        // Clean invalid data up.
        this->m_stor.bptr = nullptr;
        this->m_stor.nrsrv = 0;
        this->m_stor.nused = 0;
        return *this;
      }

    AVMC_Queue& swap(AVMC_Queue& other) noexcept
      {
        xswap(this->m_stor, other.m_stor);
        return *this;
      }

    AVMC_Queue& request(size_t nbytes)
      {
        // Reserve space for a node of size `nbytes`.
        this->do_reserve_delta(nbytes);
        return *this;
      }
    template<Executor execT> AVMC_Queue& append(ParamU paramu)
      {
        // Append a node with no parameter.
        this->do_append_trivial(execT, paramu, 0, nullptr);
        return *this;
      }
    template<Executor execT, typename XnodeT> AVMC_Queue& append(ParamU paramu, XnodeT&& xnode)
      {
        // Append a node with a parameter of type `remove_cvref_t<XnodeT>`.
        this->do_dispatch_append<execT, nullptr>(std::is_trivial<typename std::remove_reference<XnodeT>::type>(),
                                                     paramu, rocket::forward<XnodeT>(xnode));
        return *this;
      }
    template<Executor execT, Enumerator enumT, typename XnodeT> AVMC_Queue& append(ParamU paramu, XnodeT&& xnode)
      {
        // Append a node with a parameter of type `remove_cvref_t<XnodeT>`.
        this->do_dispatch_append<execT, enumT>(std::false_type(), paramu, rocket::forward<XnodeT>(xnode));
        return *this;
      }
    AVMC_Queue& append_trivial(Executor* exec, ParamU paramu, const void* data, size_t size)
      {
        // Append an arbitrary function with a trivial argument.
        this->do_append_trivial(exec, paramu, size, data);
        return *this;
      }

    AIR_Status execute(Executive_Context& ctx) const
      {
        auto status = air_status_next;
        this->do_execute_all_break(status, ctx);
        return status;
      }
    Variable_Callback& enumerate_variables(Variable_Callback& callback) const
      {
        this->do_enumerate_variables(callback);
        return callback;
      }
  };

inline void swap(AVMC_Queue& lhs, AVMC_Queue& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
