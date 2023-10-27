// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "avmc_queue.hpp"
#include "../runtime/air_node.hpp"
#include "../runtime/runtime_error.hpp"
#include "../runtime/enums.hpp"
#include "../utils.hpp"
#include <sys/mman.h>
namespace asteria {

void
AVMC_Queue::
do_reallocate(uint32_t estor)
  {
    if(estor >= 0x7FFE0000U / sizeof(Header))
      throw ::std::bad_alloc();

    ROCKET_ASSERT(estor >= this->m_einit);
    auto bptr = (Header*) ::realloc((void*) this->m_bptr, estor * sizeof(Header));
    if(!bptr)
      throw ::std::bad_alloc();

#ifdef ROCKET_DEBUG
    ::memset((void*) (bptr + this->m_einit), 0xC3, (estor - this->m_einit) * sizeof(Header));
#endif

    this->m_bptr = bptr;
    this->m_estor = estor;
  }

void
AVMC_Queue::
do_deallocate() noexcept
  {
    if(this->m_bptr) {
      this->clear();

#ifdef ROCKET_DEBUG
      ::memset((void*) this->m_bptr, 0xD9, this->m_estor * sizeof(Header));
#endif
      ::free(this->m_bptr);
    }

    this->m_bptr = nullptr;
    this->m_estor = 0;
  }

void
AVMC_Queue::
clear() noexcept
  {
    const auto eptr = this->m_bptr + this->m_einit;
    for(auto head = this->m_bptr;  head != eptr;  head += 1U + head->nheaders) {
      if(head->meta_ver == 0)
        continue;

      // Take ownership of metadata.
      unique_ptr<Metadata> meta(head->pv_meta);

      if(head->pv_meta->dtor_opt)
        head->pv_meta->dtor_opt(head);
    }

#ifdef ROCKET_DEBUG
    ::std::memset(this->m_bptr, 0xE6, this->m_estor * sizeof(Header));
#endif
    this->m_einit = 0;

    if(this->m_jit_code)
      ::munmap(this->m_jit_code, this->m_jit_size);

    this->m_jit_code = nullptr;
    this->m_jit_size = 0;
  }

details_avmc_queue::Header*
AVMC_Queue::
append(Executor* exec, Uparam uparam, size_t sparam_size, Constructor* ctor_opt,
       void* ctor_arg, Destructor* dtor_opt, Var_Getter* vget_opt,
       const Source_Location* sloc_opt)
  {
    constexpr uint32_t max_sparam_size = UINT8_MAX * sizeof(Header) - 1;
    if(sparam_size > max_sparam_size)
      ASTERIA_THROW((
          "Invalid AVMC node size (`$1` > `$2`)"),
          sparam_size, max_sparam_size);

    unique_ptr<Metadata> meta;
    uint8_t meta_ver = 0;

    if(ctor_opt || dtor_opt || vget_opt || sloc_opt) {
      // Allocate metadata for this node.
      meta.reset(new Metadata());
      meta_ver = 1;

      meta->dtor_opt = dtor_opt;
      meta->vget_opt = vget_opt;
      meta->exec = exec;

      if(sloc_opt) {
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
      ::std::memset(head->sparam, 0, sparam_size);

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
    uint32_t jit_size = 8192;
    void* jit_code = ::mmap(nullptr, jit_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ROCKET_ASSERT(jit_code != MAP_FAILED);

    ::memset(jit_code, 0xCC, jit_size);
    char* j = (char*) jit_code;

    // endbr64
    *(j++) = '\xF3';
    *(j++) = '\x0F';
    *(j++) = '\x1E';
    *(j++) = '\xFA';

    // push r12            ; align %rsp
    *(j++) = '\x41';
    *(j++) = '\x54';

    // mov r12, rdi        ; %r12 = ctx
    *(j++) = '\x49';
    *(j++) = '\x89';
    *(j++) = '\xFC';

    // jmp <code>
    *(j++) = '\xEB';
    *(j++) = '\x03';

    char* const jexit = j;

    // pop r12
    *(j++) = '\x41';
    *(j++) = '\x5C';

    // ret
    *(j++) = '\xC3';

    const auto eptr = this->m_bptr + this->m_einit;
    for(auto head = this->m_bptr;  head != eptr;  head += 1U + head->nheaders) {
      // mov rdi, r12      ; %rdi = ctx
      *(j++) = '\x4C';
      *(j++) = '\x89';
      *(j++) = '\xE7';

      // movabs rsi, <head>
      *(j++) = '\x48';
      *(j++) = '\xBE';
      ::rocket::xmemrpcpy(j, (const char*) &head, sizeof(head));

      // movabs rax, <exec>
      *(j++) = '\x48';
      *(j++) = '\xB8';
      Executor* exec = (head->meta_ver == 0) ? head->pv_exec : head->pv_meta->exec;
      ::rocket::xmemrpcpy(j, (const char*) &exec, sizeof(exec));

      // call rax
      *(j++) = '\xFF';
      *(j++) = '\xD0';

      // test al, al
      *(j++) = '\x84';
      *(j++) = '\xC0';

      // jnz <jexit>
      *(j++) = '\x0F';
      *(j++) = '\x85';
      int32_t end_offset = (int32_t) (jexit - (j + 4));
      ::rocket::xmemrpcpy(j, (const char*) &end_offset, sizeof(end_offset));
    }

    // pop r12
    *(j++) = '\x41';
    *(j++) = '\x5C';

    // ret
    *(j++) = '\xC3';

    // *** FINALIZE
    if(this->m_jit_code)
      ::munmap(this->m_jit_code, this->m_jit_size);

    ::mprotect(jit_code, jit_size, PROT_READ | PROT_EXEC);

    this->m_jit_code = jit_code;
    this->m_jit_size = jit_size;

  }

AIR_Status
AVMC_Queue::
execute(Executive_Context& ctx) const
  {
    if(!this->m_jit_code)
      return air_status_next;

    return ((AIR_Status (*)(Executive_Context*)) this->m_jit_code)(&ctx);
  }

void
AVMC_Queue::
collect_variables(Variable_HashMap& staged, Variable_HashMap& temp) const
  {
    const auto eptr = this->m_bptr + this->m_einit;
    for(auto head = this->m_bptr;  head != eptr;  head += 1U + head->nheaders) {
      if(head->meta_ver == 0)
        continue;

      if(head->pv_meta->vget_opt)
        head->pv_meta->vget_opt(staged, temp, head);
    }
  }

}  // namespace asteria
