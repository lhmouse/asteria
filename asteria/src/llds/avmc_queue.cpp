// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "avmc_queue.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/enums.hpp"
#include "../utils.hpp"

namespace asteria {

void
AVMC_Queue::
do_destroy_nodes(bool xfree) noexcept
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_used;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += UINT32_C(1) + qnode->nheaders;

      // Destroy `sparam`, if any.
      if(qnode->meta_ver && qnode->pv_meta->dtor_opt)
        qnode->pv_meta->dtor_opt(qnode);

      // Deallocate the vtable and symbols.
      if(qnode->meta_ver)
        delete qnode->pv_meta;
    }

#ifdef ROCKET_DEBUG
    ::std::memset(this->m_bptr, 0xE6, this->m_estor * sizeof(Header));
#endif

    this->m_used = 0xDEADBEEF;
    if(!xfree)
      return;

    // Deallocate the old table.
    auto bold = ::std::exchange(this->m_bptr, (Header*)0xDEADBEEF);
    auto esold = ::std::exchange(this->m_estor, (size_t)0xBEEFDEAD);
    ::rocket::freeN<Header>(bold, esold);
  }

void
AVMC_Queue::
do_reallocate(uint32_t nadd)
  {
    // Allocate a new table.
    constexpr size_t nheaders_max = UINT32_MAX / sizeof(Header);
    if(nheaders_max - this->m_used < nadd)
      throw ::std::bad_alloc();

    uint32_t estor = this->m_used + nadd;
    auto bptr = ::rocket::allocN<Header>(estor);

    // Perform a bitwise copy of all contents of the old block.
    // This copies all existent headers and trivial data.
    // Note that the size is unchanged.
    auto bold = ::std::exchange(this->m_bptr, bptr);
    ::std::memcpy(bptr, bold, this->m_used * sizeof(Header));
    auto esold = ::std::exchange(this->m_estor, estor);
    if(ROCKET_EXPECT(!bold))
      return;

    // Move old non-trivial nodes if any.
    // Warning: no exception shall be thrown from the code below.
    uint32_t offset = 0;
    while(ROCKET_EXPECT(offset != this->m_used)) {
      auto qnode = bptr + offset;
      auto qfrom = bold + offset;
      offset += UINT32_C(1) + qnode->nheaders;

      // Relocate `sparam`, if any.
      if(qnode->meta_ver && qnode->pv_meta->reloc_opt)
        qnode->pv_meta->reloc_opt(qnode, qfrom);
    }

    // Deallocate the old table.
    ::rocket::freeN<Header>(bold, esold);
  }

details_avmc_queue::Header*
AVMC_Queue::
do_reserve_one(Uparam uparam, size_t size)
  {
    constexpr size_t size_max = UINT8_MAX * sizeof(Header) - 1;
    if(size > size_max)
      ASTERIA_THROW("invalid AVMC node size (`$1` > `$2`)", size, size_max);

    // Round the size up to the nearest number of headers.
    // This shall not result in overflows.
    size_t nheaders_p1 = (sizeof(Header) * 2 - 1 + size) / sizeof(Header);

    // Allocate a new memory block as needed.
    if(ROCKET_UNEXPECT(this->m_estor - this->m_used < nheaders_p1)) {
      size_t nadd = nheaders_p1;
#ifndef ROCKET_DEBUG
      // Reserve more space for non-debug builds.
      nadd |= this->m_used * 4;
#endif
      this->do_reallocate(static_cast<uint32_t>(nadd));
    }
    ROCKET_ASSERT(this->m_estor - this->m_used >= nheaders_p1);

    // Append a new node.
    // `uparam` is overlapped with `nheaders` so it must be assigned first.
    // The others can occur in any order.
    auto qnode = this->m_bptr + this->m_used;
    qnode->uparam = uparam;
    qnode->nheaders = static_cast<uint8_t>(nheaders_p1 - UINT32_C(1));
    qnode->meta_ver = 0;
    return qnode;
  }

details_avmc_queue::Header*
AVMC_Queue::
do_append_trivial(Uparam uparam, Executor* exec, size_t size, const void* data_opt)
  {
    // Copy source data if `data_opt` is non-null. Fill zeroes otherwise.
    // This operation will not throw exceptions.
    auto qnode = this->do_reserve_one(uparam, size);
    if(data_opt)
      ::std::memcpy(qnode->sparam, data_opt, size);
    else if(size)
      ::std::memset(qnode->sparam, 0, size);

    // Accept this node.
    qnode->pv_exec = exec;
    this->m_used += UINT32_C(1) + qnode->nheaders;
    return qnode;
  }

details_avmc_queue::Header*
AVMC_Queue::
do_append_nontrivial(Uparam uparam, Executor* exec, const Source_Location* sloc_opt,
                     Var_Getter* vget_opt, Relocator* reloc_opt, Destructor* dtor_opt,
                     size_t size, Constructor* ctor_opt, intptr_t ctor_arg)
  {
    // Allocate metadata for this node.
    auto meta = ::rocket::make_unique<details_avmc_queue::Metadata>();
    uint8_t meta_ver = 1;

    meta->reloc_opt = reloc_opt;
    meta->dtor_opt = dtor_opt;
    meta->vget_opt = vget_opt;
    meta->exec = exec;

    if(sloc_opt) {
      meta->syms = *sloc_opt;
      meta_ver = 2;
    }

    // Invoke the constructor if `ctor_opt` is non-null. Fill zeroes otherwise.
    // If an exception is thrown, there is no effect.
    auto qnode = this->do_reserve_one(uparam, size);
    if(ctor_opt)
      ctor_opt(qnode, ctor_arg);
    else if(size)
      ::std::memset(qnode->sparam, 0, size);

    // Accept this node.
    qnode->pv_meta = meta.release();
    qnode->meta_ver = meta_ver;
    this->m_used += UINT32_C(1) + qnode->nheaders;
    return qnode;
  }

AVMC_Queue&
AVMC_Queue::
finalize()
  {
    // TODO: Add JIT support.
    this->do_reallocate(0);
    return *this;
  }

AIR_Status
AVMC_Queue::
execute(Executive_Context& ctx) const
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_used;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += UINT32_C(1) + qnode->nheaders;

      // Get the executor, which must always exist.
      Executor* executor = qnode->pv_exec;
      if(qnode->meta_ver != 0)
        executor = qnode->pv_meta->exec;

      ASTERIA_RUNTIME_TRY {
        auto status = executor(ctx, qnode);
        if(status != air_status_next)
          return status;
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
        if(qnode->meta_ver == 1)
          throw;

        // Symbols exist. Use them.
        except.push_frame_plain(qnode->pv_meta->syms, sref(""));
        throw;
      }
    }
    return air_status_next;
  }

void
AVMC_Queue::
get_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_used;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += UINT32_C(1) + qnode->nheaders;

      // Get the variable enumeration callback, which is optional.
      Var_Getter* vgetter = nullptr;
      if(qnode->meta_ver != 0)
        vgetter = qnode->pv_meta->vget_opt;

      if(vgetter)
        vgetter(staged, temp, qnode);
    }
  }

}  // namespace asteria
