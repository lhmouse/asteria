// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVMC_QUEUE_
#define ASTERIA_LLDS_AVMC_QUEUE_

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
    using Constructor = details_avmc_queue::Constructor;
    using Destructor  = details_avmc_queue::Destructor;
    using Var_Getter  = details_avmc_queue::Var_Getter;
    using Metadata    = details_avmc_queue::Metadata;

  private:
    Header* m_bptr = nullptr;  // beginning of storage
    uint32_t m_used = 0;       // used storage in number of `Header`s [!]
    uint32_t m_estor = 0;      // allocated storage in number of `Header`s [!]

  public:
    explicit constexpr
    AVMC_Queue() noexcept
      { }

    AVMC_Queue(AVMC_Queue&& other) noexcept
      {
        this->swap(other);
      }

    AVMC_Queue&
    operator=(AVMC_Queue&& other) & noexcept
      {
        this->swap(other);
        return *this;
      }

    AVMC_Queue&
    swap(AVMC_Queue& other) noexcept
      {
        ::std::swap(this->m_bptr, other.m_bptr);
        ::std::swap(this->m_estor, other.m_estor);
        ::std::swap(this->m_used, other.m_used);
        return *this;
      }

  private:
    void
    do_reallocate(uint32_t estor);

  public:
    ~AVMC_Queue()
      {
        if(this->m_bptr)
          this->do_reallocate(0);
      }

    bool
    empty() const noexcept
      { return this->m_used == 0;  }

    void
    clear() noexcept;

    // Append a new node to the end. `size` is the size of `sparam` to initialize.
    // If `ctor_opt` is specified, it is called to initialize `sparam`. Otherwise,
    // `sparam` is filled with zeroes. If `sloc_opt` is specified, it denotes the
    // symbols for backtracing, which are copied and stored by these functions and
    // need not be persistent.
    Header*
    append(Executor* exec, Uparam uparam, size_t sparam_size, Constructor* ctor_opt,
           void* ctor_arg, Destructor* dtor_opt, Var_Getter* vget_opt = nullptr,
           const Source_Location* sloc_opt = nullptr);

    Header*
    append(Executor* exec, Uparam uparam, size_t sparam_size, Constructor* ctor_opt,
           void* ctor_arg, Destructor* dtor_opt, const Source_Location* sloc_opt)
      {
        return this->append(exec, uparam, sparam_size,
                            ctor_opt, ctor_arg, dtor_opt, nullptr, sloc_opt);
      }

    Header*
    append(Executor* exec, size_t sparam_size, Constructor* ctor_opt, void* ctor_arg,
           Destructor* dtor_opt, Var_Getter* vget_opt = nullptr,
           const Source_Location* sloc_opt = nullptr)
      {
        return this->append(exec, Uparam(), sparam_size,
                            ctor_opt, ctor_arg, dtor_opt, vget_opt, sloc_opt);
      }

    Header*
    append(Executor* exec, size_t sparam_size, Constructor* ctor_opt, void* ctor_arg,
           Destructor* dtor_opt, const Source_Location* sloc_opt)
      {
        return this->append(exec, Uparam(), sparam_size,
                            ctor_opt, ctor_arg, dtor_opt, nullptr, sloc_opt);
      }

    Header*
    append(Executor* exec, Uparam uparam, const Source_Location* sloc_opt = nullptr)
      {
        return this->append(exec, uparam, 0,
                            nullptr, nullptr, nullptr, nullptr, sloc_opt);
      }

    Header*
    append(Executor* exec, const Source_Location* sloc_opt = nullptr)
      {
        return this->append(exec, Uparam(), 0,
                            nullptr, nullptr, nullptr, nullptr, sloc_opt);
      }

    // Marks this queue ready for execution. No nodes may be appended hereafter.
    // This function serves as an optimization hint.
    void
    finalize();

    // These are interfaces called by the runtime.
    AIR_Status
    execute(Executive_Context& ctx) const;

    void
    collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const;
  };

inline
void
swap(AVMC_Queue& lhs, AVMC_Queue& rhs) noexcept
  {
    lhs.swap(rhs);
  }

}  // namespace asteria
#endif
