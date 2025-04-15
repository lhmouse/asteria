// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LLDS_AVM_ROD_
#define ASTERIA_LLDS_AVM_ROD_

#include "../fwd.hpp"
#include "../source_location.hpp"
#include "../details/avm_rod.ipp"
namespace asteria {

class AVM_Rod
  {
  public:
    using Uparam       = details_avm_rod::Uparam;
    using Header       = details_avm_rod::Header;
    using Metadata     = details_avm_rod::Metadata;
    using Executor     = details_avm_rod::Executor;
    using Constructor  = details_avm_rod::Sparam_Constructor;
    using Destructor   = details_avm_rod::Sparam_Destructor;
    using Collector    = details_avm_rod::Collector;

  private:
    Header* m_bptr = nullptr;
    uint32_t m_einit = 0;
    uint32_t m_estor = 0;

  public:
    constexpr AVM_Rod() noexcept = default;

    AVM_Rod(AVM_Rod&& other) noexcept
      {
        this->swap(other);
      }

    AVM_Rod&
    operator=(AVM_Rod&& other) & noexcept
      {
        this->swap(other);
        return *this;
      }

    AVM_Rod&
    swap(AVM_Rod& other) noexcept
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
    ~AVM_Rod()
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
    append(Executor* exec, Uparam uparam, size_t sparam_size, Constructor* ctor_opt, void* ctor_arg,
           Destructor* dtor_opt, Collector* coll_opt, const Source_Location* sloc_opt);

    // Marks this rod ready for execution. No nodes may be appended hereafter.
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
swap(AVM_Rod& lhs, AVM_Rod& rhs) noexcept
  { lhs.swap(rhs);  }

}  // namespace asteria
#endif
