// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "avmc_queue.hpp"
#include "../runtime/runtime_error.hpp"
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

      // Destroy symbols.
      if(qnode->has_syms)
        ::rocket::destroy_at(qnode->symbols());

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
      ASTERIA_RUNTIME_TRY {
        status = (*exec)(ctx, qnode->paramu(), qnode->paramv());
      }
      ASTERIA_RUNTIME_CATCH(Runtime_Error& except) {
        if(qnode->has_syms)
          except.push_frame_plain(qnode->symbols()->sloc);
        throw;
      }
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

void AVMC_Queue::do_reserve_delta(size_t nbytes, const Symbols* syms_opt)
  {
    // Once a node has been appended, reallocation is no longer allowed.
    // Otherwise we would have to move nodes around, which complexifies things without any obvious benefits.
    if(this->m_bptr)
      ASTERIA_THROW("AVMC queue not resizable");

    // Get the maximum size of user-defined parameter that is allowed in a node.
    Header qnode[1];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Woverflow"
    qnode->nphdrs = SIZE_MAX;
#pragma GCC diagnostic pop
    size_t nphdrs_max = qnode->nphdrs;

    // Check for overlarge `nbytes` values.
    size_t nbytes_max = nphdrs_max * sizeof(Header);
    if(nbytes > nbytes_max)
      ASTERIA_THROW("invalid AVMC node size (`$1` > `$2`)", nbytes, nbytes_max);

    // Create a dummy header and calculate its size.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    qnode->nphdrs = (nbytes + sizeof(Header) - 1) / sizeof(Header);
    qnode->has_syms = !!syms_opt;
#pragma GCC diagnostic pop
    uint32_t nhdrs_total = qnode->total_size_in_headers();
    constexpr uint32_t nhdrs_max = UINT32_MAX / sizeof(Header);
    if(nhdrs_max - this->m_rsrv < nhdrs_total) {
      ASTERIA_THROW("too many AVMC nodes (`$1` + `$2` > `$3`)", this->m_rsrv, nhdrs_total, nhdrs_max);
    }
    this->m_rsrv += nhdrs_total;
  }

AVMC_Queue::Header* AVMC_Queue::do_allocate_node(ParamU paramu, const Symbols* syms_opt, size_t nbytes)
  {
    // Check space for the very first header.
    uint32_t nhdrs_avail = this->m_rsrv - this->m_used;
    if(nhdrs_avail == 0)
      ASTERIA_THROW("AVMC queue full");

    // If no storage has been allocated so far, it shall be allocated now.
    auto qnode = this->m_bptr;
    if(ROCKET_UNEXPECT(!qnode)) {
      qnode = static_cast<Header*>(::operator new(this->m_rsrv * sizeof(Header)));
      this->m_bptr = qnode;
    }
    qnode += this->m_used;

    // Create a minimal header and calculate its size.
    // The caller is responsible for setting other fields appropriately.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    qnode->nphdrs = (nbytes + sizeof(Header) - 1) / sizeof(Header);
    qnode->has_syms = !!syms_opt;
#pragma GCC diagnostic pop
    uint32_t nhdrs_total = qnode->total_size_in_headers();
    if(nhdrs_total > nhdrs_avail) {
      ASTERIA_THROW("insufficient space in AVMC queue (`$1` > `$2`)", nhdrs_total > nhdrs_avail);
    }
    qnode->paramu_x16 = paramu.x16;
    qnode->paramu_x32 = paramu.x32;
    return qnode;
  }

void AVMC_Queue::do_append_trivial(Executor* exec, ParamU paramu, const Symbols* syms_opt,
                                   const void* source, size_t nbytes)
  {
    // Create a new node.
    auto qnode = this->do_allocate_node(paramu, syms_opt, nbytes);
    qnode->has_vtbl = false;
    qnode->exec = exec;

    // Copy source data if any.
    if(nbytes != 0)
      ::std::memcpy(qnode->paramv(), source, nbytes);

    // Set up symbols. This shall not throw exceptions.
    if(syms_opt)
      ::rocket::construct_at(qnode->symbols(), *syms_opt);

    // Consume the storage.
    this->m_used += qnode->total_size_in_headers();
  }

void AVMC_Queue::do_append_nontrivial(const Vtable* vtbl, ParamU paramu, const Symbols* syms_opt,
                                      size_t nbytes, Constructor* ctor_opt, intptr_t source)
  {
    // Create a new node.
    auto qnode = this->do_allocate_node(paramu, syms_opt, nbytes);
    qnode->has_vtbl = true;
    qnode->vtbl = vtbl;

    // Invoke the constructor. If an exception is thrown, there is no effect.
    if(ROCKET_EXPECT(ctor_opt))
      (*ctor_opt)(paramu, qnode->paramv(), source);
    else if(nbytes != 0)
      ::std::memset(qnode->paramv(), 0, nbytes);

    // Set up symbols. This shall not throw exceptions.
    if(syms_opt)
      ::rocket::construct_at(qnode->symbols(), *syms_opt);

    // Consume the storage.
    this->m_used += qnode->total_size_in_headers();
  }

}  // namespace Asteria
