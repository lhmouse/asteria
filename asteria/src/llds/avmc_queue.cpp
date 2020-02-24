// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

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
    auto qnode = bptr;
    while(ROCKET_EXPECT(qnode != eptr)) {
      // Call the destructor for this node.
      auto dtor = qnode->has_vtbl ? qnode->vtbl->dtor : nullptr;
      if(ROCKET_UNEXPECT(dtor))
        (*dtor)(qnode->get_paramu(), qnode->get_paramv());
      qnode += qnode->nphdrs + size_t(1);
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
    auto qnode = bptr;
    while(ROCKET_EXPECT(qnode != eptr)) {
      // Call the executor function for this node.
      auto exec = qnode->has_vtbl ? qnode->vtbl->exec : qnode->exec;
      ROCKET_ASSERT(exec);
      status = (*exec)(ctx, qnode->get_paramu(), qnode->get_paramv());
      if(ROCKET_UNEXPECT(status != air_status_next))
        return;
      qnode += qnode->nphdrs + size_t(1);
    }
  }

void AVMC_Queue::do_enumerate_variables(Variable_Callback& callback) const
  {
    auto bptr = this->m_stor.bptr;
    auto eptr = bptr + this->m_stor.nused;
    // Enumerate variables from all nodes.
    auto qnode = bptr;
    while(ROCKET_EXPECT(qnode != eptr)) {
      // Call the enumerator function for this node.
      auto vnum = qnode->has_vtbl ? qnode->vtbl->vnum : nullptr;
      if(ROCKET_UNEXPECT(vnum))
        (*vnum)(callback, qnode->get_paramu(), qnode->get_paramv());
      qnode += qnode->nphdrs + size_t(1);
    }
  }

void AVMC_Queue::do_reserve_delta(size_t nbytes)
  {
    // Once a node has been appended, reallocation is no longer allowed.
    // Otherwise we would have to move nodes around, which complexifies things without any obvious benefits.
    if(this->m_stor.bptr) {
      ASTERIA_THROW("AVMC queue not resizable");
    }
    constexpr auto nbytes_hdr = sizeof(Header);
    constexpr auto nbytes_max = nbytes_hdr * nphdrs_max;
    if(nbytes > nbytes_max) {
      ASTERIA_THROW("invalid AVMC node size (`$1` > `$2`)", nbytes, nbytes_max);
    }
    auto nbytes_node = static_cast<uint32_t>(1 + (nbytes + nbytes_hdr - 1) / nbytes_hdr);
    // Reserve one header, followed by `nphdrs` headers for the parameters.
    if(this->m_stor.nrsrv > INT32_MAX / nbytes_hdr + nbytes_node) {
      ASTERIA_THROW("too many AVMC nodes");
    }
    this->m_stor.nrsrv += nbytes_node;
  }

AVMC_Queue::Header* AVMC_Queue::do_check_storage_for_paramv(size_t nbytes)
  {
    constexpr auto nbytes_hdr = sizeof(Header);
    auto bptr = this->m_stor.bptr;
    // If no storage has been allocated so far, it shall be allocated now.
    if(ROCKET_UNEXPECT(!bptr)) {
      bptr = static_cast<Header*>(::operator new(nbytes_hdr * this->m_stor.nrsrv));
      this->m_stor.bptr = bptr;
    }
    auto qnode = bptr + this->m_stor.nused;
    // Check the number of available headers.
    auto navail = static_cast<size_t>(this->m_stor.nrsrv - this->m_stor.nused);
    if((navail < 1) || (nbytes > nbytes_hdr * (navail - 1))) {
      ASTERIA_THROW("AVMC queue full");
    }
    qnode->nphdrs = (nbytes + nbytes_hdr - 1) / nbytes_hdr % nphdrs_max;
    return qnode;
  }

void AVMC_Queue::do_append_trivial(Executor* exec, AVMC_Queue::ParamU paramu, size_t nbytes, const void* source)
  {
    auto qnode = this->AVMC_Queue::do_check_storage_for_paramv(nbytes);
    auto nbytes_node = static_cast<uint32_t>(qnode->nphdrs + size_t(1));
    // Initialize the node.
    qnode->has_vtbl = false;
    qnode->paramu_x16 = paramu.x16;
    qnode->paramu_x32 = paramu.x32;
    qnode->exec = exec;
    // Copy source data if any.
    if(nbytes != 0) {
      ::std::memcpy(qnode->paramv, source, nbytes);
    }
    this->m_stor.nused += nbytes_node;
  }

void AVMC_Queue::do_append_nontrivial(ref_to<const Vtable> vtbl, AVMC_Queue::ParamU paramu, size_t nbytes,
                                      Constructor* ctor, intptr_t source)
  {
    auto qnode = this->AVMC_Queue::do_check_storage_for_paramv(nbytes);
    auto nbytes_node = static_cast<uint32_t>(qnode->nphdrs + size_t(1));
    // Initialize the node.
    qnode->has_vtbl = true;
    qnode->paramu_x16 = paramu.x16;
    qnode->paramu_x32 = paramu.x32;
    qnode->vtbl = vtbl.ptr();
    // Invoke the constructor if any, which is subject to exceptions.
    if(ROCKET_EXPECT(ctor))
      (*ctor)(paramu, qnode->paramv, source);
    this->m_stor.nused += nbytes_node;
  }

}  // namespace Asteria
