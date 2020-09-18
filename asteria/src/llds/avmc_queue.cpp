// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "avmc_queue.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/variable_callback.hpp"
#include "../runtime/runtime_error.hpp"
#include "../utilities.hpp"

namespace asteria {

using details_avmc_queue::Uparam;
using details_avmc_queue::Symbols;
using details_avmc_queue::Constructor;
using details_avmc_queue::Executor;
using details_avmc_queue::Enumerator;
using details_avmc_queue::Vtable;
using details_avmc_queue::Header;

void
AVMC_Queue::
do_destroy_nodes()
noexcept
  {
    auto eptr = this->m_bptr + this->m_used;
    auto next = this->m_bptr;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += qnode->total_size_in_headers();

      // Destroy user-defined data.
      if(auto qdtor = qnode->dtor_opt())
        qdtor(qnode->uparam(), qnode->sparam());

      // Destroy symbols.
      if(auto ksyms = qnode->syms_opt())
        ::rocket::destroy_at(ksyms);
    }

#ifdef ROCKET_DEBUG
    this->m_used = 0xDEADBEEF;
#endif
  }

void
AVMC_Queue::
do_reallocate(uint32_t nadd)
  {
    ROCKET_ASSERT(nadd <= UINT16_MAX);

    // Allocate a new table.
    constexpr size_t nhdrs_max = UINT32_MAX / sizeof(Header);
    if(nhdrs_max - this->m_used < nadd)
      throw ::std::bad_array_new_length();
    uint32_t rsrv = this->m_used + nadd;
    auto bptr = static_cast<Header*>(::operator new(rsrv * sizeof(Header)));

    // Performa bitwise copy of all contents of the old block.
    // This copies all existent headers and trivial data. Note that the size is unchanged.
    auto bold = ::std::exchange(this->m_bptr, bptr);
    if(this->m_used)
      ::std::memcpy(bptr, bold, this->m_used * sizeof(Header));
    this->m_rsrv = rsrv;

    // Move old non-trivial nodes if any.
    // Warning: No exception shall be thrown from the code below.
    uint32_t offset = 0;
    while(ROCKET_EXPECT(offset != this->m_used)) {
      auto qnode = bptr + offset;
      auto qxold = bold + offset;
      offset += qnode->total_size_in_headers();

      // Move-construct new user-defined data.
      if(auto qmvct = qnode->mvct_opt())
        qmvct(qnode->uparam(), qnode->sparam(), qxold->sparam());

      // Move-construct new symbols.
      if(auto ksyms = qnode->syms_opt())
        ::rocket::construct_at(ksyms, ::std::move(*(qxold->syms_opt())));

      // Destroy old user-defined data.
      if(auto qdtor = qxold->dtor_opt())
        qdtor(qxold->uparam(), qxold->sparam());

      // Destroy old symbols.
      if(auto ksyms = qxold->syms_opt())
        ::rocket::destroy_at(ksyms);
    }
    // Deallocate the old block.
    if(bold)
      ::operator delete(bold);
  }

Header*
AVMC_Queue::
do_reserve_one(Uparam uparam, const opt<Symbols>& syms_opt, size_t nbytes)
  {
    constexpr size_t nbytes_max = UINT8_MAX * sizeof(Header) - 1;
    if(nbytes > nbytes_max)
      ASTERIA_THROW("Invalid AVMC node size (`$1` > `$2`)", nbytes, nbytes_max);

    // Create a dummy header for calculation.
    Header temph;
    // Note `up_stor` is partially overlapped with other fields, so it must be assigned first.
    temph.up_stor = uparam;
    // The following may occur in any order.
    temph.nphdrs = static_cast<uint8_t>((nbytes + sizeof(Header) - 1) / sizeof(Header));
    temph.has_syms = !!syms_opt;

    // Allocate a new memory block as needed.
    uint32_t nadd = temph.total_size_in_headers();
    if(ROCKET_UNEXPECT(this->m_rsrv - this->m_used < nadd)) {
#ifndef ROCKET_DEBUG
      // Reserve more space for non-debug builds.
      nadd |= this->m_used * 4;
#endif
      this->do_reallocate(nadd);
    }
    ROCKET_ASSERT(this->m_rsrv - this->m_used >= nadd);

    // Append a new node.
    auto qnode = this->m_bptr + this->m_used;
    ::std::memcpy(qnode, &temph, sizeof(temph));
    return qnode;
  }

AVMC_Queue&
AVMC_Queue::
do_append_trivial(Executor* exec, Uparam uparam, opt<Symbols>&& syms_opt,
                  size_t nbytes, const void* src_opt)
  {
    auto qnode = this->do_reserve_one(uparam, syms_opt, nbytes);
    qnode->has_vtbl = false;
    qnode->exec = exec;

    // Copy source data if `src_opt` is non-null. Fill zeroes otherwise.
    // This operation will not throw exceptions.
    if(src_opt)
      ::std::memcpy(qnode->sparam(), src_opt, nbytes);
    else
      ::std::memset(qnode->sparam(), 0, nbytes);

    // Set up symbols.
    // This operation will not throw exceptions.
    if(auto qsyms = qnode->syms_opt())
      ::rocket::construct_at(qsyms, ::std::move(*syms_opt));

    // Accept this node.
    this->m_used += qnode->total_size_in_headers();
    return *this;
  }

AVMC_Queue&
AVMC_Queue::
do_append_nontrivial(const Vtable* vtbl, Uparam uparam, opt<Symbols>&& syms_opt,
                     size_t nbytes, Constructor* ctor_opt, intptr_t ctor_arg)
  {
    auto qnode = this->do_reserve_one(uparam, syms_opt, nbytes);
    qnode->has_vtbl = true;
    qnode->vtable = vtbl;

    // Invoke the constructor if `ctor_opt` is non-null. Fill zeroes otherwise.
    // If an exception is thrown, there is no effect.
    if(ctor_opt)
      ctor_opt(uparam, qnode->sparam(), ctor_arg);
    else
      ::std::memset(qnode->sparam(), 0, nbytes);

    // Set up symbols.
    // This operation will not throw exceptions.
    if(auto qsyms = qnode->syms_opt())
      ::rocket::construct_at(qsyms, ::std::move(*syms_opt));

    // Accept this node.
    this->m_used += qnode->total_size_in_headers();
    return *this;
  }

AIR_Status
AVMC_Queue::
execute(Executive_Context& ctx)
const
  {
    auto eptr = this->m_bptr + this->m_used;
    auto next = this->m_bptr;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += qnode->total_size_in_headers();

      // Call the executor function for this node.
      AIR_Status status;
      ASTERIA_RUNTIME_TRY {
        status = qnode->executor()(ctx, qnode->uparam(), qnode->sparam());
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
        if(auto qsyms = qnode->syms_opt())
          except.push_frame_plain(qsyms->sloc, ::rocket::sref(""));
        throw;
      }
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
    auto eptr = this->m_bptr + this->m_used;
    auto next = this->m_bptr;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = next;
      next += qnode->total_size_in_headers();

      // Enumerate variables in this node.
      if(auto qvenum = qnode->venum_opt())
        qvenum(callback, qnode->uparam(), qnode->sparam());
    }
    return callback;
  }

}  // namespace asteria
