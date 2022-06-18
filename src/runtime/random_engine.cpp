// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "random_engine.hpp"
#include "../utils.hpp"
#include <fcntl.h>  // ::open()
#include <unistd.h>  // ::read()

namespace asteria {

Random_Engine::
~Random_Engine()
  {
  }

void
Random_Engine::
do_isaac() noexcept
  {
    ///////////////////////////////////////////////////////////////////////////
    // Below is a direct copy with a few fixups.
    //   https://www.burtleburtle.net/bob/c/rand.c
    ///////////////////////////////////////////////////////////////////////////

#define ind(mm,x)  (*(uint32_t *)((uint8_t *)(mm) + ((x) & (255<<2))))
#define rngstep(mix,a,b,mm,m,m2,r,x) \
    { \
      x = *m;  \
      a = (a^(mix)) + *(m2++); \
      *(m++) = y = ind(mm,x) + a + b; \
      *(r++) = b = ind(mm,y>>8) + x; \
    }

    uint32_t a,b,x,y,*m,*mm,*m2,*r,*mend;
    mm=this->m_randmem; r=this->m_randrsl;
    a = this->m_randa; b = this->m_randb + (++this->m_randc);
    for (m = mm, mend = m2 = m+128; m<mend; )
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
    this->m_randb = b; this->m_randa = a;

    ///////////////////////////////////////////////////////////////////////////
    // End of copied code
    ///////////////////////////////////////////////////////////////////////////
  }

void
Random_Engine::
init() noexcept
  {
    // Initialize internal states.
    this->m_randa = 0;
    this->m_randb = 0;
    this->m_randc = 0;
    ::std::memset(this->m_randrsl, 0, sizeof(m_randrsl));

    // Read some random bytes. Errors are ignored.
    ::rocket::unique_posix_fd fd(::open("/dev/urandom", O_RDONLY));
    if(fd)
      (void)!::read(fd, this->m_randrsl, sizeof(m_randrsl));

    ///////////////////////////////////////////////////////////////////////////
    // Below is a direct copy with a few fixups.
    //   https://www.burtleburtle.net/bob/c/rand.c
    ///////////////////////////////////////////////////////////////////////////

#define mix(a,b,c,d,e,f,g,h) \
    { \
      a^=b<<11; d+=a; b+=c; \
      b^=c>>2;  e+=b; c+=d; \
      c^=d<<8;  f+=c; d+=e; \
      d^=e>>16; g+=d; e+=f; \
      e^=f<<10; h+=e; f+=g; \
      f^=g>>4;  a+=f; g+=h; \
      g^=h<<8;  b+=g; h+=a; \
      h^=a>>9;  c+=h; a+=b; \
    }

    uint32_t i;
    uint32_t a,b,c,d,e,f,g,h;
    uint32_t *m,*r;
    this->m_randa = this->m_randb = this->m_randc = 0;
    m=this->m_randmem;
    r=this->m_randrsl;
    a=b=c=d=e=f=g=h=0x9e3779b9;  /* the golden ratio */

    for (i=0; i<4; ++i)          /* scramble it */
    {
      mix(a,b,c,d,e,f,g,h);
    }

    /* initialize using the contents of r[] as the seed */
    for (i=0; i<256; i+=8)
    {
      a+=r[i  ]; b+=r[i+1]; c+=r[i+2]; d+=r[i+3];
      e+=r[i+4]; f+=r[i+5]; g+=r[i+6]; h+=r[i+7];
      mix(a,b,c,d,e,f,g,h);
      m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
      m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
    }
    /* do a second pass to make all of the seed affect all of m */
    for (i=0; i<256; i+=8)
    {
      a+=m[i  ]; b+=m[i+1]; c+=m[i+2]; d+=m[i+3];
      e+=m[i+4]; f+=m[i+5]; g+=m[i+6]; h+=m[i+7];
      mix(a,b,c,d,e,f,g,h);
      m[i  ]=a; m[i+1]=b; m[i+2]=c; m[i+3]=d;
      m[i+4]=e; m[i+5]=f; m[i+6]=g; m[i+7]=h;
    }

    ///////////////////////////////////////////////////////////////////////////
    // End of copied code
    ///////////////////////////////////////////////////////////////////////////

    // Discard results of the first round.
    this->do_isaac();
    this->m_randcnt = 0;
  }

}  // namespace asteria
