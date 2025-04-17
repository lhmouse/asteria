// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "avm_rod.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/enums.hpp"
#include "../utils.hpp"
namespace asteria {

ROCKET_FLATTEN
void
AVM_Rod::
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

ROCKET_FLATTEN
void
AVM_Rod::
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
AVM_Rod::
clear() noexcept
  {
    ptrdiff_t offset = -(ptrdiff_t) this->m_einit;
    while(offset != 0) {
      auto head = this->m_bptr + this->m_einit + offset;
      offset += 1L + head->nheaders;

      if(head->meta_ver == 0)
        continue;

      if(head->pv_meta->dtor_opt)
        (* head->pv_meta->dtor_opt) (head);

      delete head->pv_meta;
    }

#ifdef ROCKET_DEBUG
    if(this->m_bptr)
      ::memset(this->m_bptr, 0xE6, this->m_estor * sizeof(Header));
#endif
    this->m_einit = 0;
  }

AVM_Rod::Header*
AVM_Rod::
push_function(Executor* exec, Uparam uparam, size_t sparam_size, Constructor* ctor_opt,
              void* ctor_arg, Destructor* dtor_opt, Collector* coll_opt,
              const Source_Location* sloc_opt)
  {
    constexpr size_t max_sparam_size = UINT8_MAX * sizeof(Header) - 1;
    if(sparam_size > max_sparam_size)
       ::rocket::sprintf_and_throw<::std::invalid_argument>(
             "asteria::AVM_Rod: `sparam_size` too large (`%zd` > `%zd`)",
             sparam_size, max_sparam_size);

    unique_ptr<Metadata> meta;
    uint8_t meta_ver = 0;

    if(ctor_opt || dtor_opt || coll_opt || sloc_opt) {
      meta.reset(new Metadata());
      meta_ver = 1;

      // ver. 1
      meta->exec = exec;
      meta->dtor_opt = dtor_opt;
      meta->coll_opt = coll_opt;

      if(sloc_opt) {
        // ver. 2
        meta->sloc = *sloc_opt;
        meta_ver = 2;
      }
    }

    // Round the size up to the nearest number of headers. This shall not result
    // in overflows.
    uint32_t nheaders_p1 = (uint32_t) (1 + (sparam_size + sizeof(Header) - 1) / sizeof(Header));
#ifdef ROCKET_DEBUG
    this->do_reallocate(this->m_einit + nheaders_p1);
#else
    if(ROCKET_UNEXPECT(nheaders_p1 > this->m_estor - this->m_einit))
      this->do_reallocate(this->m_einit * 2 + nheaders_p1 + 5);
#endif

    // Append a new node. `uparam` is overlapped with `nheaders` so it must be
    // assigned first. The others can occur in any order.
    auto head = this->m_bptr + this->m_einit;
    head->uparam = uparam;
    head->nheaders = (uint8_t) (nheaders_p1 - 1);

    if(ctor_opt)
      (*ctor_opt) (head, ctor_arg);
    else if(sparam_size != 0)
      ::memset(head->sparam, 0, sparam_size);

    if(meta)
      head->pv_meta = meta.release();
    else
      head->pv_exec = exec;

    head->meta_ver = meta_ver;
    this->m_einit += nheaders_p1;
    return head;
  }

void
AVM_Rod::
finalize()
  {
    // TODO: Add JIT support.
  }

void
AVM_Rod::
execute(AIR_Status& status, Executive_Context& ctx) const
  {
    status = air_status_next;
    ptrdiff_t offset = -(ptrdiff_t) this->m_einit;
    while(ROCKET_EXPECT(status == air_status_next) && (offset != 0)) {
      auto head = this->m_bptr + this->m_einit + offset;
      offset += 1L + head->nheaders;

      try {
        if(head->meta_ver == 0)
          status = (* head->pv_exec) (ctx, head);
        else
          status = (* head->pv_meta->exec) (ctx, head);
      }
      catch(Runtime_Error& except) {
        // Modify and rethrow the exception in place without copying it.
        if(head->meta_ver >= 2)
          except.push_frame_plain(head->pv_meta->sloc);
        throw;
      }
      catch(exception& stdex) {
        // Replace the current exception.
        Runtime_Error except(xtc_format, "$1", stdex);
        if(head->meta_ver >= 2)
          except.push_frame_plain(head->pv_meta->sloc);
        throw except;
      }
    }
  }

void
AVM_Rod::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    ptrdiff_t offset = -(ptrdiff_t) this->m_einit;
    while(offset != 0) {
      auto head = this->m_bptr + this->m_einit + offset;
      offset += 1L + head->nheaders;

      if(head->meta_ver == 0)
        continue;

      if(head->pv_meta->coll_opt)
        (* head->pv_meta->coll_opt) (staged, temp, head);
    }
  }

}  // namespace asteria
