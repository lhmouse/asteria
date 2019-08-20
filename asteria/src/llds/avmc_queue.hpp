// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_HPP_
#define ASTERIA_LLDS_AVMC_QUEUE_HPP_

#include "../fwd.hpp"

namespace Asteria {

class AVMC_Queue
  {
  public:
    // These are prototypes for callbacks.
    using Constructor  = void (uint32_t paramk, void* params, intptr_t source);
    using Destructor   = void (uint32_t paramk, void* params);
    using Executor     = AIR_Status (Executive_Context& ctx, uint32_t paramk, const void* params);
    using Enumerator   = Variable_Callback& (Variable_Callback& callback, uint32_t paramk, const void* params);

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
        uint32_t nphdrs : 8;  // size of `params`, in number of `sizeof(Header)` (!)
        uint32_t has_vtbl : 1;  // vtable exists?
        uint32_t reserved : 23;
        uint32_t paramk;  // user-defined data (1)
        union {
          Executor* exec;  // active if `has_vtbl`
          const Vtable* vtbl;  // active otherwise
        };
        alignas(max_align_t) intptr_t params[];  // user-defined data (2)
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
      : m_stor()
      {
      }
    AVMC_Queue(AVMC_Queue&& other) noexcept
      : m_stor()
      {
        std::swap(this->m_stor, other.m_stor);
      }
    AVMC_Queue& operator=(AVMC_Queue&& other) noexcept
      {
        std::swap(this->m_stor, other.m_stor);
        return *this;
      }
    ~AVMC_Queue()
      {
        if(this->m_stor.bptr) {
          this->do_deallocate_storage();
        }
#ifdef ROCKET_DEBUG
        std::memset(std::addressof(this->m_stor), 0xE5, sizeof(this->m_stor));
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
    void do_append_trivial(Executor* exec, uint32_t paramk, size_t nbytes, const void* source);
    void do_append_nontrivial(ref_to<const Vtable> vtbl, uint32_t paramk, size_t nbytes, Constructor* ctor, intptr_t source);

    template<Executor execuT, nullptr_t, typename XnodeT> void do_dispatch_append(std::true_type, uint32_t paramk, XnodeT&& xnode)
      {
        // The parameter type is trivial and no vtable is required.
        // Append a node with a trivial parameter.
        this->do_append_trivial(execuT, paramk, sizeof(xnode), std::addressof(xnode));
      }
    template<Executor execuT, Enumerator* enumeraT, typename XnodeT> void do_dispatch_append(std::false_type, uint32_t paramk, XnodeT&& xnode)
      {
        // The vtable must have static storage duration. As it is defined `constexpr` here, we need 'real' function pointers.
        // Those converted from non-capturing lambdas are not an option.
        struct Fs
          {
            static void construct(uint32_t /*paramk*/, void* params, intptr_t source)
              {
                // Construct the bound parameter using perfect forwarding.
                rocket::construct_at(static_cast<typename rocket::remove_cvref<XnodeT>::type*>(params),
                                     rocket::forward<XnodeT>(*(reinterpret_cast<typename std::remove_reference<XnodeT>::type*>(source))));
              }
            static void destroy(uint32_t /*paramk*/, void* params) noexcept
              {
                // Destroy the bound parameter.
                rocket::destroy_at(static_cast<typename rocket::remove_cvref<XnodeT>::type*>(params));
              }
          };
        static constexpr Vtable s_vtbl =
          {
            Fs::destroy, execuT, enumeraT
          };
        // Append a node with a non-trivial parameter.
        this->do_append_nontrivial(rocket::ref(s_vtbl), paramk, sizeof(xnode), Fs::construct, reinterpret_cast<intptr_t>(std::addressof(xnode)));
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

    void swap(AVMC_Queue& other) noexcept
      {
        std::swap(this->m_stor, other.m_stor);
      }

    AVMC_Queue& request()
      {
        // Record a node with no parameter.
        this->do_reserve_delta(0);
        return *this;
      }
    template<typename XnodeT> AVMC_Queue& request(const XnodeT& xnode)
      {
        // Record a node with a parameter of type `remove_cvref_t<XnodeT>`.
        this->do_reserve_delta(sizeof(xnode));
        return *this;
      }
    template<Executor execuT> AVMC_Queue& append(uint32_t paramk)
      {
        // Append a node with no parameter.
        this->do_append_trivial(execuT, paramk, 0, nullptr);
        return *this;
      }
    template<Executor execuT, typename XnodeT> AVMC_Queue& append(uint32_t paramk, XnodeT&& xnode)
      {
        // Append a node with a parameter of type `remove_cvref_t<XnodeT>`.
        this->do_dispatch_append<execuT, nullptr>(std::is_trivial<typename std::remove_reference<XnodeT>::type>(), paramk, rocket::forward<XnodeT>(xnode));
        return *this;
      }
    template<Executor execuT, Enumerator enumeraT, typename XnodeT> AVMC_Queue& append(uint32_t paramk, XnodeT&& xnode)
      {
        // Append a node with a parameter of type `remove_cvref_t<XnodeT>`.
        this->do_dispatch_append<execuT, enumeraT>(std::false_type(), paramk, rocket::forward<XnodeT>(xnode));
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
    return lhs.swap(rhs);
  }

}  // namespace Asteria

#endif
