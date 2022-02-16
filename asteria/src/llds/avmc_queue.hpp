// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_HPP_
#define ASTERIA_LLDS_AVMC_QUEUE_HPP_

#include "../fwd.hpp"
#include "../source_location.hpp"
#include "../details/avmc_queue.ipp"

namespace asteria {

class AVMC_Queue
  {
  public:
    using Uparam      = details_avmc_queue::Uparam;
    using Header      = details_avmc_queue::Header;
    using Executor    = details_avmc_queue::Executor;
    using Var_Getter  = details_avmc_queue::Var_Getter;

  private:
    using Metadata     = details_avmc_queue::Metadata;
    using Constructor  = details_avmc_queue::Constructor;
    using Relocator    = details_avmc_queue::Relocator;
    using Destructor   = details_avmc_queue::Destructor;

    Header* m_bptr = nullptr;  // beginning of storage
    uint32_t m_used = 0;       // used storage in number of `Header`s [!]
    uint32_t m_estor = 0;      // allocated storage in number of `Header`s [!]

  public:
    explicit constexpr
    AVMC_Queue() noexcept
      { }

    AVMC_Queue(AVMC_Queue&& other) noexcept
      { this->swap(other);  }

    AVMC_Queue&
    operator=(AVMC_Queue&& other) noexcept
      { return this->swap(other);  }

    AVMC_Queue&
    swap(AVMC_Queue& other) noexcept
      { ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_estor, other.m_estor);
        ::std::swap(this->m_used, other.m_used);
        return *this;  }

  private:
    void
    do_destroy_nodes(bool xfree) noexcept;

    void
    do_reallocate(uint32_t nadd);

    // Reserve storage for the next node. `size` is the size of `sparam` to initialize.
    inline Header*
    do_reserve_one(Uparam uparam, size_t size);

    // Append a new node to the end. `size` is the size of `sparam` to initialize.
    // If `data_opt` is specified, it should point to the buffer containing data to copy.
    // Otherwise, `sparam` is filled with zeroes.
    Header*
    do_append_trivial(Uparam uparam, Executor* exec, size_t size, const void* data_opt);

    // Append a new node to the end. `size` is the size of `sparam` to initialize.
    // If `ctor_opt` is specified, it is called to initialize `sparam`. Otherwise,
    // `sparam` is filled with zeroes.
    Header*
    do_append_nontrivial(Uparam uparam, Executor* exec, const Source_Location* sloc_opt,
                         Var_Getter* vget_opt, Relocator* reloc_opt, Destructor* dtor_opt,
                         size_t size, Constructor* ctor_opt, intptr_t ctor_arg);

  public:
    ~AVMC_Queue()
      {
        if(this->m_bptr)
          this->do_destroy_nodes(true);
      }

    bool
    empty() const noexcept
      { return this->m_used == 0;  }

    AVMC_Queue&
    clear() noexcept
      {
        if(this->m_used)
          this->do_destroy_nodes(false);

        // Clean invalid data up.
        this->m_used = 0;
        return *this;
      }

    // Append a node. This allows you to bind an arbitrary function.
    template<typename XSparamT>
    Header*
    append(Executor& exec, const Source_Location* sloc_opt, Uparam up, XSparamT&& sp)
      {
        using Sparam = typename ::std::decay<XSparamT>::type;
        static_assert(::std::is_nothrow_move_constructible<Sparam>::value);
        using Traits = details_avmc_queue::Sparam_traits<Sparam>;

        if(::std::is_trivial<Sparam>::value && !Traits::vget_opt && !sloc_opt)
          return this->do_append_trivial(up, exec, sizeof(sp), ::std::addressof(sp));

        return this->do_append_nontrivial(up, exec, sloc_opt,
                          Traits::vget_opt, Traits::reloc_opt, Traits::dtor_opt,
                          sizeof(sp), details_avmc_queue::do_forward_ctor<XSparamT>,
                          reinterpret_cast<intptr_t>(::std::addressof(sp)));
      }

    Header*
    append(Executor& exec, const Source_Location* sloc_opt, Uparam up = { })
      {
        if(!sloc_opt)
          return this->do_append_trivial(up, exec, 0, nullptr);

        return this->do_append_nontrivial(up, exec, sloc_opt,
                           nullptr, nullptr, nullptr, 0, nullptr, 0);
      }

    // Mark this queue ready for execution. No nodes may be appended hereafter.
    // This function serves as an optimization hint.
    AVMC_Queue&
    finalize();

    // These are interfaces called by the runtime.
    AIR_Status
    execute(Executive_Context& ctx) const;

    void
    get_variables(Variable_HashMap& staged, Variable_HashMap& temp) const;
  };

inline void
swap(AVMC_Queue& lhs, AVMC_Queue& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria

#endif
