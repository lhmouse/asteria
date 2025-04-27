// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "random_engine.hpp"
#include "../utils.hpp"
#include <fcntl.h>  // ::open()
#include <unistd.h>  // ::read()
#include <openssl/rand.h>
namespace asteria {

Random_Engine::
Random_Engine() noexcept
  {
    ::RAND_priv_bytes(this->m_ctx_init, sizeof(this->m_ctx_init));
    this->m_ctx.randcnt = 256;
  }

Random_Engine::
~Random_Engine()
  {
  }

void
Random_Engine::
do_isaac() noexcept
  {
    // https://www.burtleburtle.net/bob/c/rand.c
#define ctx  (&(this->m_ctx))
#define ub1  uint8_t
#define ub2  uint16_t
#define ub4  uint32_t
#define RANDSIZL   (8)
#define RANDSIZ    (1<<RANDSIZL)
/////////////////////////////////////////////////////////////////////////////
#define ind(mm,x)  (*(ub4 *)((ub1 *)(mm) + ((x) & ((RANDSIZ-1)<<2))))
#define rngstep(mix,a,b,mm,m,m2,r,x) \
{ \
  x = *m;  \
  a = (a^(mix)) + *(m2++); \
  *(m++) = y = ind(mm,x) + a + b; \
  *(r++) = b = ind(mm,y>>RANDSIZL) + x; \
}

   /*register*/ ub4 a,b,x,y,*m,*mm,*m2,*r,*mend;
   mm=ctx->randmem; r=ctx->randrsl;
   a = ctx->randa; b = ctx->randb + (++ctx->randc);
   for (m = mm, mend = m2 = m+(RANDSIZ/2); m<mend; )
   {
      rngstep( a<<13, a, b, mm, m, m2, r, x);
      rngstep( a>>6 , a, b, mm, m, m2, r, x);
      rngstep( a<<2 , a, b, mm, m, m2, r, x);
      rngstep( a>>16, a, b, mm, m, m2, r, x);
   }
   for (m2 = mm; m2<mend; )
   {
      rngstep( a<<13, a, b, mm, m, m2, r, x);
      rngstep( a>>6 , a, b, mm, m, m2, r, x);
      rngstep( a<<2 , a, b, mm, m, m2, r, x);
      rngstep( a>>16, a, b, mm, m, m2, r, x);
   }
   ctx->randb = b; ctx->randa = a;
/////////////////////////////////////////////////////////////////////////////
  }

}  // namespace asteria
