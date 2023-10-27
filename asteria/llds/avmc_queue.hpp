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
    using Uparam              = details_avmc_queue::Uparam;
    using Header              = details_avmc_queue::Header;
    using Metadata            = details_avmc_queue::Metadata;
    using Executor            = details_avmc_queue::Executor;
    using Constructor         = details_avmc_queue::Sparam_Constructor;
    using Destructor          = details_avmc_queue::Sparam_Destructor;
    using Variable_Collector  = details_avmc_queue::Variable_Collector;

  private:
    Header* m_bptr = nullptr;  // beginning of storage
    uint32_t m_einit = 0;  // used storage in number of `Header`s [!]
    uint32_t m_estor = 0;  // allocated storage in number of `Header`s [!]

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
        ::std::swap(this->m_einit, other.m_einit);
        ::std::swap(this->m_estor, other.m_estor);
        return *this;
      }

  private:
    void
    do_reallocate(uint32_t estor);

    void
    do_deallocate() noexcept;

  public:
    ~AVMC_Queue()
      {
        if(this->m_bptr)
          this->do_deallocate();
      }

    bool
    empty() const noexcept
      { return this->m_einit == 0;  }

    void
    clear() noexcept;

    // Append a new node to the end. `size` is the size of `sparam` to initialize.
    // If `ctor_opt` is specified, it is called to initialize `sparam`. Otherwise,
    // `sparam` is filled with zeroes. If `sloc_opt` is specified, it denotes the
    // symbols for backtracing, which are copied and stored by these functions and
    // need not be persistent.
    Header*
    append(Executor* exec, Uparam uparam, size_t sparam_size, Constructor* ctor_opt,
           void* ctor_arg, Destructor* dtor_opt, Variable_Collector* vcoll_opt,
           const Source_Location* sloc_opt);

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
