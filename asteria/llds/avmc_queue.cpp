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

    ROCKET_ASSERT(estor >= this->m_einit);
    ::rocket::xmeminfo minfo;
    minfo.element_size = sizeof(Header);
    minfo.count = estor;
    ::rocket::xmemalloc(minfo);

    if(this->m_bptr) {
      ::rocket::xmeminfo rinfo;
      rinfo.element_size = sizeof(Header);
      rinfo.data = this->m_bptr;
      rinfo.count = this->m_einit;
      ::rocket::xmemcopy(minfo, rinfo);

      rinfo.count = this->m_estor;
      ::rocket::xmemfree(rinfo);
    }

    this->m_bptr = (Header*) minfo.data;
    this->m_estor = (uint32_t) minfo.count;
  }

void
AVMC_Queue::
do_deallocate() noexcept
  {
    this->clear();

    ::rocket::xmeminfo rinfo;
    rinfo.element_size = sizeof(Header);
    rinfo.data = this->m_bptr;
    rinfo.count = this->m_estor;
    ::rocket::xmemfree(rinfo);

    this->m_bptr = nullptr;
    this->m_estor = 0;
  }

void
AVMC_Queue::
clear() noexcept
  {
    ptrdiff_t offset = -(ptrdiff_t) this->m_einit;
    while(offset != 0) {
      auto head = this->m_bptr + this->m_einit + offset;
      offset += 1L + head->nheaders;

      if(head->meta_ver == 0)
        continue;

      // Take ownership of metadata.
      unique_ptr<Metadata> meta(head->pv_meta);

      if(head->pv_meta->dtor_opt)
        head->pv_meta->dtor_opt(head);
    }

#ifdef ROCKET_DEBUG
    ::memset(this->m_bptr, 0xE6, this->m_estor * sizeof(Header));
#endif
    this->m_einit = 0;
  }

details_avmc_queue::Header*
AVMC_Queue::
append(Executor* exec, Uparam uparam, size_t sparam_size, Constructor* ctor_opt,
       void* ctor_arg, Destructor* dtor_opt, Variable_Collector* vcoll_opt,
       const Source_Location* sloc_opt)
  {
    constexpr size_t max_sparam_size = UINT8_MAX * sizeof(Header) - 1;
    if(sparam_size > max_sparam_size)
       ::rocket::sprintf_and_throw<::std::invalid_argument>(
             "asteria::AVMC_Queue: `sparam_size` too large (`%zd` > `%zd`)",
             sparam_size, max_sparam_size);

    unique_ptr<Metadata> meta;
    uint8_t meta_ver = 0;

    if(ctor_opt || dtor_opt || vcoll_opt || sloc_opt) {
      meta.reset(new Metadata());
      meta_ver = 1;

      // ver. 1
      meta->exec = exec;
      meta->dtor_opt = dtor_opt;
      meta->vcoll_opt = vcoll_opt;

      if(sloc_opt) {
        // ver. 2
        meta->sloc = *sloc_opt;
        meta_ver = 2;
      }
    }

    // Round the size up to the nearest number of headers. This shall not result
    // in overflows.
    uint32_t nheaders_p1 = (uint32_t) ((sizeof(Header) * 2 - 1 + sparam_size) / sizeof(Header));
    if(this->m_estor - this->m_einit < nheaders_p1) {
      // Extend the storage.
      uint32_t size_to_reserve = this->m_einit + nheaders_p1;
#ifndef ROCKET_DEBUG
      size_to_reserve |= this->m_einit * 3;
#endif
      this->do_reallocate(size_to_reserve);
      ROCKET_ASSERT(this->m_estor - this->m_einit >= nheaders_p1);
    }

    // Append a new node. `uparam` is overlapped with `nheaders` so it must
    // be assigned first. The others can occur in any order.
    auto head = this->m_bptr + this->m_einit;
    head->uparam = uparam;
    head->nheaders = (uint8_t) (nheaders_p1 - 1);

    if(ctor_opt)
      ctor_opt(head, ctor_arg);
    else if(sparam_size != 0)
      ::memset(head->sparam, 0, sparam_size);

    if(!meta)
      head->pv_exec = exec;
    else
      head->pv_meta = meta.release();

    head->meta_ver = meta_ver;
    this->m_einit += nheaders_p1;
    return head;
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
    AIR_Status status = air_status_next;
    ptrdiff_t offset = -(ptrdiff_t) this->m_einit;
    while(offset != 0) {
      auto head = this->m_bptr + this->m_einit + offset;
      offset += 1L + head->nheaders;

      switch(head->meta_ver) {
        case 0:
          // There is no metadata or symbols.
          status = head->pv_exec(ctx, head);
          break;

        case 1:
          // There is metadata without symbols.
          status = head->pv_meta->exec(ctx, head);
          break;

        default:
          try {
            try {
              // There is metadata and symbols.
              status = head->pv_meta->exec(ctx, head);
            }
            catch(Runtime_Error& except)
            { throw;  }  // forward
            catch(exception& stdex)
            { throw Runtime_Error(Runtime_Error::M_format(), "$1", stdex);  }  // replace
          }
          catch(Runtime_Error& except) {
            // Modify and rethrow the exception in place without copying it.
            except.push_frame_plain(head->pv_meta->sloc);
            throw;
          }
          break;
      }

      if(ROCKET_UNEXPECT(status != air_status_next))
        break;
    }
    return status;
  }

void
AVMC_Queue::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    ptrdiff_t offset = -(ptrdiff_t) this->m_einit;
    while(offset != 0) {
      auto head = this->m_bptr + this->m_einit + offset;
      offset += 1L + head->nheaders;

      if(head->meta_ver == 0)
        continue;

      if(head->pv_meta->vcoll_opt)
        head->pv_meta->vcoll_opt(staged, temp, head);
    }
  }

}  // namespace asteria
