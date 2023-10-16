// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "avmc_queue.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/enums.hpp"
#include "../utils.hpp"
namespace asteria {

void
AVMC_Queue::
do_reallocate(uint32_t estor)
  {
    Header* bptr = nullptr;

    if(estor != 0) {
      // Extend the storage.
      ROCKET_ASSERT(estor >= this->m_used);

      if(estor >= 0x7FFF000U / sizeof(Header))
        throw ::std::bad_alloc();

      bptr = (Header*) ::realloc((void*) this->m_bptr, estor * sizeof(Header));
      if(!bptr)
        throw ::std::bad_alloc();
    }
    else {
      // Free the storage.
      auto next = this->m_bptr;
      const auto eptr = this->m_bptr + this->m_used;
      while(ROCKET_EXPECT(next != eptr)) {
        auto qnode = next;
        next += 1U + qnode->nheaders;

        if(qnode->meta_ver != 0) {
          unique_ptr<Metadata> meta(qnode->pv_meta);

          // If `sparam` has a destructor, invoke it.
          if(qnode->pv_meta->dtor_opt)
            qnode->pv_meta->dtor_opt(qnode);
        }
      }

#ifdef ROCKET_DEBUG
      ::memset((void*) this->m_bptr, 0xD9, this->m_estor * sizeof(Header));
#endif
      ::free(this->m_bptr);
      this->m_used = 0;
    }

    this->m_bptr = bptr;
    this->m_estor = estor;
  }

details_avmc_queue::Header*
AVMC_Queue::
do_allocate_node_storage(Uparam uparam, size_t size)
  {
    constexpr size_t size_max = UINT8_MAX * sizeof(Header) - 1;
    if(size > size_max)
      ASTERIA_THROW((
          "Invalid AVMC node size (`$1` > `$2`)"),
          size, size_max);

    // Round the size up to the nearest number of headers. This shall not result
    // in overflows.
    uint32_t nheaders_p1 = ((uint32_t) (sizeof(Header) * 2 - 1 + size) / sizeof(Header));
    if(this->m_estor - this->m_used < nheaders_p1) {
      // Extend the storage.
      uint32_t size_to_reserve = this->m_used + nheaders_p1;
#ifndef ROCKET_DEBUG
      size_to_reserve |= this->m_used * 3;
#endif
      this->do_reallocate(size_to_reserve);
    }
    ROCKET_ASSERT(this->m_estor - this->m_used >= nheaders_p1);

    // Append a new node. `uparam` is overlapped with `nheaders` so it must
    // be assigned first. The others can occur in any order.
    auto qnode = this->m_bptr + this->m_used;
    qnode->uparam = uparam;
    qnode->nheaders = (uint8_t) (nheaders_p1 - 1U);
    return qnode;
  }

details_avmc_queue::Header*
AVMC_Queue::
do_append_trivial(Uparam uparam, Executor* exec, size_t size, const void* data_opt)
  {
    // Copy source data if `data_opt` is non-null. Fill zeroes otherwise.
    // This operation will not throw exceptions.
    auto qnode = this->do_allocate_node_storage(uparam, size);
    if(data_opt)
      ::std::memcpy(qnode->sparam, data_opt, size);
    else if(size)
      ::std::memset(qnode->sparam, 0, size);

    // Accept this node.
    qnode->pv_exec = exec;
    qnode->meta_ver = 0;
    this->m_used += 1U + qnode->nheaders;
    return qnode;
  }

details_avmc_queue::Header*
AVMC_Queue::
do_append_nontrivial(Uparam uparam, Executor* exec, const Source_Location* sloc_opt,
                     Var_Getter* vget_opt, Destructor* dtor_opt, size_t size,
                     Constructor* ctor_opt, intptr_t ctor_arg)
  {
    // Allocate metadata for this node.
    unique_ptr<Metadata> meta(new Metadata);
    uint8_t meta_ver = 1;

    meta->dtor_opt = dtor_opt;
    meta->vget_opt = vget_opt;
    meta->exec = exec;

    if(sloc_opt) {
      meta->syms = *sloc_opt;
      meta_ver = 2;
    }

    // Invoke the constructor if `ctor_opt` is non-null. Fill zeroes otherwise.
    // If an exception is thrown, there is no effect.
    auto qnode = this->do_allocate_node_storage(uparam, size);
    if(ctor_opt)
      ctor_opt(qnode, ctor_arg);
    else if(size)
      ::std::memset(qnode->sparam, 0, size);

    // Accept this node.
    qnode->pv_meta = meta.release();
    qnode->meta_ver = meta_ver;
    this->m_used += 1U + qnode->nheaders;
    return qnode;
  }

void
AVMC_Queue::
clear() noexcept
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_used;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += 1U + qnode->nheaders;

      if(qnode->meta_ver != 0) {
        unique_ptr<Metadata> meta(qnode->pv_meta);

        // If `sparam` has a destructor, invoke it.
        if(qnode->pv_meta->dtor_opt)
          qnode->pv_meta->dtor_opt(qnode);
      }
    }

#ifdef ROCKET_DEBUG
    ::std::memset(this->m_bptr, 0xE6, this->m_estor * sizeof(Header));
#endif
    this->m_used = 0;
  }

void
AVMC_Queue::
finalize()
  {
    // TODO: Add JIT support.
  }

AIR_Status
AVMC_Queue::
execute(Executive_Context& ctx) const
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_used;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += 1U + qnode->nheaders;
      AIR_Status status;

      switch(qnode->meta_ver) {
        case 0:
          // There is no metadata or symbols.
          try {
            status = qnode->pv_exec(ctx, qnode);
            break;
          }
          catch(Runtime_Error& except) {
            // Forward the exception.
            throw;
          }
          catch(exception& stdex) {
            // Replace the active exception.
            Runtime_Error except(Runtime_Error::M_native(), cow_string(stdex.what()));
            throw except;
          }

        case 1:
          // There is metadata without symbols.
          try {
            status = qnode->pv_meta->exec(ctx, qnode);
            break;
          }
          catch(Runtime_Error& except) {
            // Forward the exception.
            throw;
          }
          catch(exception& stdex) {
            // Replace the active exception.
            Runtime_Error except(Runtime_Error::M_native(), cow_string(stdex.what()));
            throw except;
          }

        default:
          // There is metadata and symbols.
          try {
            status = qnode->pv_meta->exec(ctx, qnode);
            break;
          }
          catch(Runtime_Error& except) {
            // Modify the exception in place and rethrow it without copying it.
            except.push_frame_plain(qnode->pv_meta->syms, sref(""));
            throw;
          }
          catch(exception& stdex) {
            // Replace the active exception.
            Runtime_Error except(Runtime_Error::M_native(), cow_string(stdex.what()));
            except.push_frame_plain(qnode->pv_meta->syms, sref(""));
            throw except;
          }
      }

      if(status != air_status_next)
        return status;
    }
    return air_status_next;
  }

void
AVMC_Queue::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_used;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += 1U + qnode->nheaders;

      if(qnode->meta_ver == 0)
        continue;

      // Invoke the variable enumeration callback.
      if(qnode->pv_meta->vget_opt)
        qnode->pv_meta->vget_opt(staged, temp, qnode);
    }
  }

}  // namespace asteria
