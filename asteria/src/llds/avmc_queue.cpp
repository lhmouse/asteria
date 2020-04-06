// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "avmc_queue.hpp"
#include "../runtime/variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

void AVMC_Queue::do_deallocate_storage() const
  {
    auto bptr = this->m_bptr;
    auto eptr = bptr + this->m_used;

    // Destroy all nodes.
    auto qnode = bptr;
    while(ROCKET_EXPECT(qnode != eptr)) {
      // Call the destructor for this node.
      auto dtor = qnode->has_vtbl ? qnode->vtbl->dtor : nullptr;
      if(ROCKET_UNEXPECT(dtor))
        (*dtor)(qnode->paramu(), qnode->paramv());

      // Move to the next node.
      qnode += qnode->total_size_in_headers();
    }

    // Deallocate the storage if any.
    if(bptr)
      ::operator delete(bptr);
  }

void AVMC_Queue::do_execute_all_break(AIR_Status& status, Executive_Context& ctx) const
  {
    auto bptr = this->m_bptr;
    auto eptr = bptr + this->m_used;

    // Execute all nodes.
    auto qnode = bptr;
    while(ROCKET_EXPECT(qnode != eptr)) {
      // Call the executor function for this node.
      auto exec = qnode->has_vtbl ? qnode->vtbl->exec : qnode->exec;
      ROCKET_ASSERT(exec);
      status = (*exec)(ctx, qnode->paramu(), qnode->paramv());
      if(ROCKET_UNEXPECT(status != air_status_next))
        return;

      // Move to the next node.
      qnode += qnode->total_size_in_headers();
    }
  }

void AVMC_Queue::do_enumerate_variables(Variable_Callback& callback) const
  {
    auto bptr = this->m_bptr;
    auto eptr = bptr + this->m_used;

    // Enumerate variables from all nodes.
    auto qnode = bptr;
    while(ROCKET_EXPECT(qnode != eptr)) {
      // Call the enumerator function for this node.
      auto vnum = qnode->has_vtbl ? qnode->vtbl->vnum : nullptr;
      if(ROCKET_UNEXPECT(vnum))
        (*vnum)(callback, qnode->paramu(), qnode->paramv());

      // Move to the next node.
      qnode += qnode->total_size_in_headers();
    }
  }

void AVMC_Queue::do_reserve_delta(size_t nbytes)
  {
    // Once a node has been appended, reallocation is no longer allowed.
    // Otherwise we would have to move nodes around, which complexifies things without any obvious benefits.
    if(this->m_bptr)
      ASTERIA_THROW("AVMC queue not resizable");

    // Get the maximum size of user-defined parameter that is allowed in a node.
    Header test;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Woverflow"
    test.nphdrs = SIZE_MAX;
#pragma GCC diagnostic pop
    size_t nphdrs_max = test.nphdrs;

    // Check for overlarge `nbytes` values.
    constexpr size_t nbytes_hdr = sizeof(Header);
    size_t nbytes_max = nbytes_hdr * nphdrs_max;
    if(nbytes > nbytes_max)
      ASTERIA_THROW("invalid AVMC node size (`$1` > `$2`)", nbytes, nbytes_max);

    // Reserve one header, followed by `nphdrs` headers for the parameters.
    uint32_t nhdrs_total = static_cast<uint32_t>((nbytes_hdr * 2 - 1 + nbytes) / nbytes_hdr);
    constexpr uint32_t nhdrs_max = UINT32_MAX / nbytes_hdr;
    if(this->m_rsrv > nhdrs_max - nhdrs_total)
      ASTERIA_THROW("too many AVMC nodes (`$1` + `$2` > `$3`)", this->m_rsrv, nhdrs_total, nhdrs_max);
    this->m_rsrv += nhdrs_total;
  }

AVMC_Queue::Header* AVMC_Queue::do_check_node_storage(size_t nbytes)
  {
    // Check the number of available headers.
    constexpr size_t nbytes_hdr = sizeof(Header);
    uint32_t nhdrs_total = static_cast<uint32_t>((nbytes_hdr * 2 - 1 + nbytes) / nbytes_hdr);
    uint32_t nhdrs_avail = this->m_rsrv - this->m_used;
    if(nhdrs_total > nhdrs_avail)
      ASTERIA_THROW("AVMC queue full (`$1` > `$2`)", nhdrs_total > nhdrs_avail);

    // If no storage has been allocated so far, it shall be allocated now.
    auto qnode = this->m_bptr;
    if(ROCKET_UNEXPECT(!qnode)) {
      qnode = static_cast<Header*>(::operator new(this->m_rsrv * nbytes_hdr));
      this->m_bptr = qnode;
    }
    qnode += this->m_used;

    // Initialize the `nphdrs` field here only.
    // The caller is responsible for setting other fields appropriately.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    qnode->nphdrs = nhdrs_total - 1;
#pragma GCC diagnostic pop
    return qnode;
  }

void AVMC_Queue::do_append_trivial(Executor* exec, AVMC_Queue::ParamU paramu, size_t nbytes, const void* source)
  {
    // Create a new node.
    auto qnode = this->do_check_node_storage(nbytes);
    qnode->has_vtbl = false;
    qnode->paramu_x16 = paramu.x16;
    qnode->paramu_x32 = paramu.x32;
    qnode->exec = exec;

    // Copy source data if any.
    if(nbytes != 0)
      ::std::memcpy(qnode->paramv(), source, nbytes);

    // Consume the storage.
    this->m_used += qnode->total_size_in_headers();
  }

void AVMC_Queue::do_append_nontrivial(const AVMC_Queue::Vtable* vtbl, AVMC_Queue::ParamU paramu, size_t nbytes,
                                      AVMC_Queue::Constructor* ctor_opt, intptr_t source)
  {
    // Create a new node.
    auto qnode = this->do_check_node_storage(nbytes);
    qnode->has_vtbl = true;
    qnode->paramu_x16 = paramu.x16;
    qnode->paramu_x32 = paramu.x32;
    qnode->vtbl = vtbl;

    // Invoke the constructor. If an exception is thrown, there is no effect.
    if(ROCKET_EXPECT(ctor_opt))
      (*ctor_opt)(paramu, qnode->paramv(), source);

    // Consume the storage.
    this->m_used += qnode->total_size_in_headers();
  }

}  // namespace Asteria
