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
    if(estor >= 0x7FFE0000U / sizeof(Header))
      throw ::std::bad_alloc();

    ROCKET_ASSERT(estor >= this->m_used);
    auto bptr = (Header*) ::realloc((void*) this->m_bptr, estor * sizeof(Header));
    if(!bptr)
      throw ::std::bad_alloc();

#ifdef ROCKET_DEBUG
    ::memset((void*) (bptr + this->m_used), 0xC3, (estor - this->m_used) * sizeof(Header));
#endif

    this->m_bptr = bptr;
    this->m_estor = estor;
  }

void
AVMC_Queue::
do_deallocate() noexcept
  {
    if(this->m_bptr) {
      this->clear();

#ifdef ROCKET_DEBUG
      ::memset((void*) this->m_bptr, 0xD9, this->m_estor * sizeof(Header));
#endif
      ::free(this->m_bptr);
    }

    this->m_bptr = nullptr;
    this->m_estor = 0;
  }

void
AVMC_Queue::
clear() noexcept
  {
    auto next = this->m_bptr;
    while(ROCKET_EXPECT(next != this->m_bptr + this->m_used)) {
      auto qnode = next;
      next += 1U + qnode->nheaders;

      if(qnode->meta_ver == 0)
        continue;

      // Take ownership of metadata.
      unique_ptr<Metadata> meta(qnode->pv_meta);

      if(qnode->pv_meta->dtor_opt)
        qnode->pv_meta->dtor_opt(qnode);
    }

#ifdef ROCKET_DEBUG
    ::std::memset(this->m_bptr, 0xE6, this->m_estor * sizeof(Header));
#endif
    this->m_used = 0;
  }

details_avmc_queue::Header*
AVMC_Queue::
append(Executor* exec, Uparam uparam, size_t sparam_size, Constructor* ctor_opt,
       void* ctor_arg, Destructor* dtor_opt, Var_Getter* vget_opt,
       const Source_Location* sloc_opt)
  {
    size_t max_sparam_size = UINT8_MAX * sizeof(Header) - 1U;
    if(sparam_size > max_sparam_size)
      ASTERIA_THROW((
          "Invalid AVMC node size (`$1` > `$2`)"),
          sparam_size, max_sparam_size);

    unique_ptr<Metadata> meta;
    uint8_t meta_ver = 0;

    if(ctor_opt || dtor_opt || vget_opt || sloc_opt) {
      // Allocate metadata for this node.
      meta.reset(new Metadata());
      meta_ver = 1;

      meta->dtor_opt = dtor_opt;
      meta->vget_opt = vget_opt;
      meta->exec = exec;

      if(sloc_opt) {
        meta->sloc = *sloc_opt;
        meta_ver = 2;
      }
    }

    // Round the size up to the nearest number of headers. This shall not result
    // in overflows.
    size_t nheaders_p1 = (sizeof(Header) * 2U - 1U + sparam_size) / sizeof(Header);
    if(this->m_estor - this->m_used < nheaders_p1) {
      // Extend the storage.
      uint32_t size_to_reserve = this->m_used + (uint32_t) nheaders_p1;
#ifndef ROCKET_DEBUG
      size_to_reserve |= this->m_used * 3;
#endif
      this->do_reallocate(size_to_reserve);
      ROCKET_ASSERT(this->m_estor - this->m_used >= nheaders_p1);
    }

    // Append a new node. `uparam` is overlapped with `nheaders` so it must
    // be assigned first. The others can occur in any order.
    auto qnode = this->m_bptr + this->m_used;
    qnode->uparam = uparam;
    qnode->nheaders = (uint8_t) (nheaders_p1 - 1U);

    if(ctor_opt)
      ctor_opt(qnode, ctor_arg);
    else if(sparam_size != 0)
      ::std::memset(qnode->sparam, 0, sparam_size);

    if(!meta)
      qnode->pv_exec = exec;
    else
      qnode->pv_meta = meta.release();

    qnode->meta_ver = meta_ver;
    this->m_used += 1U + qnode->nheaders;
    return qnode;
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
    while(ROCKET_EXPECT(next != this->m_bptr + this->m_used)) {
      auto qnode = next;
      next += 1U + qnode->nheaders;
      AIR_Status status;

      switch(qnode->meta_ver) {
        case 0:
          // There is no metadata or symbols.
          status = qnode->pv_exec(ctx, qnode);
          break;

        case 1:
          // There is metadata without symbols.
          status = qnode->pv_meta->exec(ctx, qnode);
          break;

        default:
          try {
            // There is metadata and symbols.
            status = qnode->pv_meta->exec(ctx, qnode);
            break;
          }
          catch(Runtime_Error& except) {
            // Modify the exception in place and rethrow it without copying it.
            except.push_frame_plain(qnode->pv_meta->sloc);
            throw;
          }
          catch(exception& stdex) {
            // Replace the active exception.
            Runtime_Error except(Runtime_Error::M_format(), "$1", stdex);
            except.push_frame_plain(qnode->pv_meta->sloc);
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
    while(ROCKET_EXPECT(next != this->m_bptr + this->m_used)) {
      auto qnode = next;
      next += 1U + qnode->nheaders;

      if(qnode->meta_ver == 0)
        continue;

      if(qnode->pv_meta->vget_opt)
        qnode->pv_meta->vget_opt(staged, temp, qnode);
    }
  }

}  // namespace asteria
