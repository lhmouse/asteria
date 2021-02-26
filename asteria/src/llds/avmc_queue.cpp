// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "avmc_queue.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/variable_callback.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/enums.hpp"
#include "../utils.hpp"

namespace asteria {

void
AVMC_Queue::
do_destroy_nodes()
  noexcept
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_used;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += UINT32_C(1) + qnode->nheaders;

      // Destroy `sparam`, if any.
      if(qnode->meta_ver && qnode->pv_meta->dtor_opt)
        qnode->pv_meta->dtor_opt(qnode->uparam, qnode->sparam);

      // Deallocate the vtable and symbols.
      if(qnode->meta_ver)
        delete qnode->pv_meta;
    }
#ifdef ROCKET_DEBUG
    ::std::memset(this->m_bptr, 0xE6, this->m_rsrv * sizeof(Header));
#endif
    this->m_used = 0xDEADBEEF;
  }

void
AVMC_Queue::
do_reallocate(uint32_t nadd)
  {
    ROCKET_ASSERT(nadd <= UINT16_MAX);

    // Allocate a new table.
    constexpr size_t nheaders_max = UINT32_MAX / sizeof(Header);
    if(nheaders_max - this->m_used < nadd)
      throw ::std::bad_array_new_length();

    uint32_t rsrv = this->m_used + nadd;
    auto bptr = static_cast<Header*>(::operator new(rsrv * sizeof(Header)));

    // Perform a bitwise copy of all contents of the old block.
    // This copies all existent headers and trivial data.
    // Note that the size is unchanged.
    auto bold = ::std::exchange(this->m_bptr, bptr);
    ::std::memcpy(bptr, bold, this->m_used * sizeof(Header));
    this->m_rsrv = rsrv;

    // Move old non-trivial nodes if any.
    // Warning: No exception shall be thrown from the code below.
    uint32_t offset = 0;
    while(ROCKET_EXPECT(offset != this->m_used)) {
      auto qnode = bptr + offset;
      auto qfrom = bold + offset;
      offset += UINT32_C(1) + qnode->nheaders;

      // Relocate `sparam`, if any.
      if(qnode->meta_ver && qnode->pv_meta->reloc_opt)
        qnode->pv_meta->reloc_opt(qnode->uparam, qnode->sparam, qfrom->sparam);
    }
    if(bold)
      ::operator delete(bold);
  }

details_avmc_queue::Header*
AVMC_Queue::
do_reserve_one(Uparam uparam, size_t size)
  {
    constexpr size_t size_max = UINT8_MAX * sizeof(Header) - 1;
    if(size > size_max)
      ASTERIA_THROW("Invalid AVMC node size (`$1` > `$2`)", size, size_max);

    // Allocate a new memory block as needed.
    size_t nadd = (sizeof(Header) * 2 - 1 + size) / sizeof(Header);
    if(ROCKET_UNEXPECT(this->m_rsrv - this->m_used < nadd)) {
#ifndef ROCKET_DEBUG
      // Reserve more space for non-debug builds.
      nadd |= this->m_used * 4;
#endif
      this->do_reallocate(static_cast<uint32_t>(nadd));
    }
    ROCKET_ASSERT(this->m_rsrv - this->m_used >= nadd);

    // Append a new node.
    // `uparam` is overlapped with `nheaders` so it must be assigned first.
    // The others can occur in any order.
    auto qnode = this->m_bptr + this->m_used;
    qnode->uparam = uparam;
    qnode->nheaders = static_cast<uint8_t>(nadd - UINT32_C(1));
    qnode->meta_ver = 0;
    return qnode;
  }

AVMC_Queue&
AVMC_Queue::
do_append_trivial(Uparam uparam, Executor* exec, const void* data, size_t size)
  {
    auto qnode = this->do_reserve_one(uparam, size);

    // Copy source data if `data` is non-null. Fill zeroes otherwise.
    // This operation will not throw exceptions.
    if(data)
      ::std::memcpy(qnode->sparam, data, size);
    else
      ::std::memset(qnode->sparam, 0, size);

    // Accept this node.
    qnode->pv_exec = exec;
    this->m_used += UINT32_C(1) + qnode->nheaders;
    return *this;
  }

AVMC_Queue&
AVMC_Queue::
do_append_nontrivial(Uparam uparam, Executor* exec, const Source_Location* sloc_opt,
                     Enumerator* enum_opt, Relocator* reloc_opt, Destructor* dtor_opt,
                     Constructor* ctor_opt, size_t size, intptr_t ctor_arg)
  {
    auto qnode = this->do_reserve_one(uparam, size);

    // Allocate metadata for this node.
    auto meta = ::rocket::make_unique<details_avmc_queue::Metadata>();
    uint8_t meta_ver = 1;

    meta->reloc_opt = reloc_opt;
    meta->dtor_opt = dtor_opt;
    meta->enum_opt = enum_opt;
    meta->exec = exec;

    if(sloc_opt) {
      meta->syms = *sloc_opt;
      meta_ver = 2;
    }

    // Invoke the constructor if `ctor_opt` is non-null. Fill zeroes otherwise.
    // If an exception is thrown, there is no effect.
    if(ctor_opt)
      ctor_opt(uparam, qnode->sparam, size, ctor_arg);
    else
      ::std::memset(qnode->sparam, 0, size);

    // Accept this node.
    qnode->pv_meta = meta.release();
    qnode->meta_ver = meta_ver;
    this->m_used += UINT32_C(1) + qnode->nheaders;
    return *this;
  }

AIR_Status
AVMC_Queue::
execute(Executive_Context& ctx)
  const
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_used;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += UINT32_C(1) + qnode->nheaders;

      AIR_Status status;
      if(ROCKET_EXPECT(qnode->meta_ver >= 2)) {
        // Symbols are available.
        ASTERIA_RUNTIME_TRY {
          status = qnode->pv_meta->exec(ctx, qnode->uparam, qnode->sparam);
        }
        ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
          except.push_frame_plain(qnode->pv_meta->syms, sref(""));
          throw;
        }
      }
      else if(qnode->meta_ver == 1)
        status = qnode->pv_meta->exec(ctx, qnode->uparam, qnode->sparam);
      else
        status = qnode->pv_exec(ctx, qnode->uparam, qnode->sparam);

      if(ROCKET_UNEXPECT(status != air_status_next))
        return status;
    }
    return air_status_next;
  }

Variable_Callback&
AVMC_Queue::
enumerate_variables(Variable_Callback& callback)
  const
  {
    auto next = this->m_bptr;
    const auto eptr = this->m_bptr + this->m_used;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += UINT32_C(1) + qnode->nheaders;

      // Enumerate variables from this node.
      if(qnode->meta_ver && qnode->pv_meta->enum_opt)
        qnode->pv_meta->enum_opt(callback, qnode->uparam, qnode->sparam);
    }
    return callback;
  }

}  // namespace asteria
