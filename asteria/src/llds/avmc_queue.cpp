// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "avmc_queue.hpp"
#include "../runtime/variable_callback.hpp"
#include "../utilities.hpp"

namespace Asteria {

void AVMC_Queue::do_deallocate_storage() const
  {
    auto bptr = this->m_stor.bptr;
    auto eptr = bptr + this->m_stor.nused;
    // Destroy all nodes.
    auto next = bptr;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = std::exchange(next, next + 1 + next->nphdrs);
      // Call the destructor for this node.
      auto dtor = qnode->has_vtbl ? qnode->vtbl->dtor : nullptr;
      if(ROCKET_UNEXPECT(dtor)) {
        (*dtor)(qnode->paramb, qnode->paramk, qnode->params);
      }
    }
    // Deallocate the storage if any.
    if(bptr) {
      ::operator delete(bptr);
    }
  }

void AVMC_Queue::do_execute_all_break(AIR_Status& status, Executive_Context& ctx) const
  {
    auto bptr = this->m_stor.bptr;
    auto eptr = bptr + this->m_stor.nused;
    // Execute all nodes.
    auto next = bptr;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = std::exchange(next, next + 1 + next->nphdrs);
      // Call the executor function for this node.
      auto exec = qnode->has_vtbl ? qnode->vtbl->exec : qnode->exec;
      ROCKET_ASSERT(exec);
      status = (*exec)(ctx, qnode->paramb, qnode->paramk, qnode->params);
      if(ROCKET_UNEXPECT(status != air_status_next)) {
        // Forward any status codes unexpected to the caller.
        return;
      }
    }
  }

void AVMC_Queue::do_enumerate_variables(Variable_Callback& callback) const
  {
    auto bptr = this->m_stor.bptr;
    auto eptr = bptr + this->m_stor.nused;
    // Enumerate variables from all nodes.
    auto next = bptr;
    while(ROCKET_EXPECT(next != eptr)) {
      auto qnode = std::exchange(next, next + 1 + next->nphdrs);
      // Call the enumerator function for this node.
      auto vnum = qnode->has_vtbl ? qnode->vtbl->vnum : nullptr;
      if(ROCKET_UNEXPECT(vnum)) {
        (*vnum)(callback, qnode->paramb, qnode->paramk, qnode->params);
      }
    }
  }

void AVMC_Queue::do_reserve_delta(size_t nbytes)
  {
    // Once a node has been appended, reallocation is no longer allowed.
    // Otherwise we would have to move nodes around, which complexifies things without any obvious benefits.
    if(this->m_stor.bptr) {
      ASTERIA_THROW_RUNTIME_ERROR("Reservation is no longer allowed once an AVMC node has been appended.");
    }
    constexpr auto nbytes_hdr = sizeof(Header);
    constexpr auto nbytes_max = nbytes_hdr * nphdrs_max;
    if(nbytes > nbytes_max) {
      ASTERIA_THROW_RUNTIME_ERROR("AMVC node parameter size `", nbytes, "` exceeds the limit `", nbytes_max, "`.");
    }
    auto nbytes_node = static_cast<uint32_t>(1 + (nbytes + nbytes_hdr - 1) / nbytes_hdr);
    // Reserve one header, followed by `nphdrs` headers for the parameters.
    if(this->m_stor.nrsrv > INT32_MAX / nbytes_hdr + nbytes_node) {
      ASTERIA_THROW_RUNTIME_ERROR("Too many AVMC nodes have been appended.");
    }
    this->m_stor.nrsrv += nbytes_node;
  }

AVMC_Queue::Header* AVMC_Queue::do_check_storage_for_params(size_t nbytes)
  {
    constexpr auto nbytes_hdr = sizeof(Header);
    auto bptr = this->m_stor.bptr;
    // If no storage has been allocated so far, it shall be allocated now, forbidding further reservation.
    if(ROCKET_UNEXPECT(!bptr)) {
      bptr = static_cast<Header*>(::operator new(nbytes_hdr * this->m_stor.nrsrv));
      this->m_stor.bptr = bptr;
    }
    auto qnode = bptr + this->m_stor.nused;
    // Check the number of available headers.
    auto navail = static_cast<size_t>(this->m_stor.nrsrv - this->m_stor.nused);
    if((navail < 1) || (nbytes_hdr * (navail - 1) < nbytes)) {
      ASTERIA_THROW_RUNTIME_ERROR("This AVMC queue is full.");
    }
    qnode->nphdrs = (nbytes + nbytes_hdr - 1) / nbytes_hdr % nphdrs_max;
    return qnode;
  }

void AVMC_Queue::do_append_trivial(Executor* exec, uint8_t paramb, uint32_t paramk, size_t nbytes, const void* source)
  {
    auto qnode = this->AVMC_Queue::do_check_storage_for_params(nbytes);
    auto nbytes_node = static_cast<uint32_t>(1 + qnode->nphdrs);
    // Initialize the node.
    qnode->has_vtbl = false;
    qnode->paramb = paramb;
    qnode->paramk = paramk;
    qnode->exec = exec;
    // Copy source data if any.
    if(nbytes != 0) {
      std::memcpy(qnode->params, source, nbytes);
    }
    // Finish the initialization.
    this->m_stor.nused += nbytes_node;
  }

void AVMC_Queue::do_append_nontrivial(ref_to<const Vtable> vtbl, uint8_t paramb, uint32_t paramk, size_t nbytes, Constructor* ctor, intptr_t source)
  {
    auto qnode = this->AVMC_Queue::do_check_storage_for_params(nbytes);
    auto nbytes_node = static_cast<uint32_t>(1 + qnode->nphdrs);
    // Initialize the node.
    qnode->has_vtbl = true;
    qnode->paramb = paramb;
    qnode->paramk = paramk;
    qnode->vtbl = vtbl.ptr();
    // Invoke the constructor if any, which is subject to exceptions.
    if(ROCKET_EXPECT(ctor)) {
      (*ctor)(paramb, paramk, qnode->params, source);
    }
    // Finish the initialization.
    this->m_stor.nused += nbytes_node;
  }

}  // namespace Asteria
