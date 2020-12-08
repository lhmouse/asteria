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
      next += qnode->total_size_in_headers();

      // Destroy symbols and user-defined data.
      qnode->destroy();
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
    constexpr size_t nhdrs_max = UINT32_MAX / sizeof(Header);
    if(nhdrs_max - this->m_used < nadd)
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
      offset += qnode->total_size_in_headers();

      // Relocate user-defined data.
      qnode->relocate(::std::move(*qfrom));
    }
    if(bold)
      ::operator delete(bold);
  }

details_avmc_queue::Header*
AVMC_Queue::
do_reserve_one(Uparam uparam, const opt<Symbols>& syms_opt, size_t nbytes)
  {
    constexpr size_t nbytes_max = UINT8_MAX * sizeof(Header) - 1;
    if(nbytes > nbytes_max)
      ASTERIA_THROW("Invalid AVMC node size (`$1` > `$2`)", nbytes, nbytes_max);

    // Create a dummy header for calculation.
    // Note `up_stor` is partially overlapped with other fields, so it must be assigned
    // first. The others may occur in any order.
    Header temph;
    temph.up_stor = uparam;
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
do_append_trivial(Executor* executor, Uparam uparam, opt<Symbols>&& syms_opt,
                  size_t nbytes, const void* src_opt)
  {
    auto qnode = this->do_reserve_one(uparam, syms_opt, nbytes);
    qnode->has_vtbl = false;
    qnode->executor = executor;

    // Copy symbols.
    uptr<Symbols> usyms;
    if(syms_opt)
      usyms = ::rocket::make_unique<Symbols>(::std::move(*syms_opt));

    // Copy source data if `src_opt` is non-null. Fill zeroes otherwise.
    // This operation will not throw exceptions.
    if(src_opt)
      ::std::memcpy(qnode->sparam(), src_opt, nbytes);
    else
      ::std::memset(qnode->sparam(), 0, nbytes);

    // Set up symbols.
    // This operation will not throw exceptions.
    if(usyms)
      qnode->adopt_symbols(::std::move(usyms));

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
    qnode->vtbl = vtbl;

    // Copy symbols.
    uptr<Symbols> usyms;
    if(syms_opt)
      usyms = ::rocket::make_unique<Symbols>(::std::move(*syms_opt));

    // Invoke the constructor if `ctor_opt` is non-null. Fill zeroes otherwise.
    // If an exception is thrown, there is no effect.
    if(ctor_opt)
      ctor_opt(uparam, qnode->sparam(), ctor_arg);
    else
      ::std::memset(qnode->sparam(), 0, nbytes);

    // Set up symbols.
    // This operation will not throw exceptions.
    if(usyms)
      qnode->adopt_symbols(::std::move(usyms));

    // Accept this node.
    this->m_used += qnode->total_size_in_headers();
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
      next += qnode->total_size_in_headers();

      // Call the executor function for this node.
      ASTERIA_RUNTIME_TRY {
        auto status = qnode->execute(ctx);
        if(ROCKET_UNEXPECT(status != air_status_next))
          return status;
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
        qnode->push_symbols(except);
        throw;
      }
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
      next += qnode->total_size_in_headers();

      // Enumerate variables in this node.
      qnode->enumerate_variables(callback);
    }
    return callback;
  }

}  // namespace asteria
