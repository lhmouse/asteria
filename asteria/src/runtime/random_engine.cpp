// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "random_engine.hpp"
#include "../utilities.hpp"
#include <fcntl.h>  // ::open()
#include <unistd.h>  // ::close(), ::read()

namespace Asteria {
namespace {

constexpr
uint32_t
do_sll(uint32_t reg, int bits)
noexcept
  { return reg << bits;  }

constexpr
uint32_t
do_srl(uint32_t reg, int bits)
noexcept
  { return reg >> bits;  }

size_t
do_read_random_device(void* data, size_t size)
noexcept
  {
    ::rocket::unique_posix_fd fd(::open("/dev/urandom", O_RDONLY), ::close);
    if(!fd)
      return 0;
    ::ssize_t nread = ::read(fd, data, size);
    if(nread < 0)
      return 0;
    return static_cast<size_t>(nread);
  }

}  // namespace

Random_Engine::
~Random_Engine()
  {
  }

void
Random_Engine::
do_update()
noexcept
  {
    // Increment `cc` and combine it with `bb` for every round.
    this->m_cc += 1;
    this->m_bb += this->m_cc;

    // Unroll the loop by 4.
    for(size_t i = 0;  i < 64;  ++i) {
      auto step = [&](size_t r, auto shift, int b)
        {
          auto x = this->m_mm[r];
          auto y = this->m_aa;
          y ^= shift(y, b);
          y += this->m_mm[(r + 128) % 256];
          this->m_aa = y;
          y += this->m_mm[(x >> 2) % 256] + this->m_bb;
          this->m_mm[r] = y;
          x += this->m_mm[(y >> 10) % 256];
          this->m_bb = x;
          this->m_pool[r] = x;
        };

      step(i * 4 + 0, do_sll, 13);
      step(i * 4 + 1, do_srl,  6);
      step(i * 4 + 2, do_sll,  2);
      step(i * 4 + 3, do_srl, 16);
    }

    // Mark this round ready for consumption.
    this->m_ngot = 0;
  }

void
Random_Engine::
init()
noexcept
  {
    // Initialize internal states.
    this->m_aa = 0;
    this->m_bb = 0;
    this->m_cc = 0;

    // Initialize the pool with some entropy from the system.
    ::std::memset(this->m_mm, '/', sizeof(m_mm));
    do_read_random_device(this->m_mm, sizeof(m_mm));

    // Discard the result of the very first two rounds.
    this->do_update();
    this->do_update();

    this->m_ngot = 0;
  }

uint32_t
Random_Engine::
bump()
noexcept
  {
    // Advance the index.
    uint32_t k = this->m_ngot;
    this->m_ngot = (k + 1) % 256;

    // Populate the pool with new numbers if the pool has been exhausted.
    if(ROCKET_UNEXPECT(k != this->m_ngot))
      this->do_update();

    return this->m_pool[k];
  }

}  // namespace Asteria
