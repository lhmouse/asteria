// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "random_number_generator.hpp"
#include "../utilities.hpp"
#ifdef _WIN32
#  include <minwindef.h>  // so many...
#  include <ntsecapi.h>  // RtlGenRandom()
#else
#  include <fcntl.h>  // open()
#  include <unistd.h>  // read()
#endif

namespace Asteria {

Random_Number_Generator::~Random_Number_Generator()
  {
  }

void Random_Number_Generator::do_update() noexcept
  {
    // Increment `cc` and combine it with `bb` for every round.
    this->m_cc += 1;
    this->m_bb += this->m_cc;
    // Unroll the loop by 4.
    for(unsigned i = 0; i != 256; i += 4) {
      auto step = [&](unsigned r, auto&& aa_spec)
        {
          auto x = this->m_mm[r];
          this->m_aa ^= aa_spec();
          this->m_aa += this->m_mm[(r + 128) % 256];
          auto y = this->m_mm[(x >> 2) % 256] + this->m_aa + this->m_bb;
          this->m_mm[r] = y;
          this->m_bb = this->m_mm[(y >> 10) % 256] + x;
          this->m_pool[r] = this->m_bb;
        };
      step(i + 0, [&] { return this->m_aa << 13;  });
      step(i + 1, [&] { return this->m_aa >>  6;  });
      step(i + 2, [&] { return this->m_aa <<  2;  });
      step(i + 3, [&] { return this->m_aa >> 16;  });
    }
    // Mark this round ready for consumption.
    this->m_ngot = 0;
  }

    namespace {

    bool do_random_fill(void* data, std::size_t size) noexcept
      {
return std::memset(data, 0, size);
#ifdef _WIN32
        return ::RtlGenRandom(data, static_cast<std::uint32_t>(size));
#else
        int fd = ::open("/dev/urandom", O_RDONLY);
        if(fd == -1) {
          return false;
        }
        auto nread = ::read(fd, data, size);
        ::close(fd);
        return nread > 0;
#endif
      }

    }

void Random_Number_Generator::reset() noexcept
  {
    // Initialize internal state data.
    this->m_aa = 0;
    this->m_bb = 0;
    this->m_cc = 0;
    do_random_fill(this->m_mm, sizeof(this->m_mm));
    // Scramble the states.
    std::uint32_t regs[8];
    for(unsigned i = 0; i != 8; i += 1) {
      regs[i] = 0x9E3779B9;
    }
    auto mix_regs = [&]()
      {
        regs[0] ^= regs[1] << 11, regs[3] += regs[0], regs[1] += regs[2];
        regs[1] ^= regs[2] >>  2, regs[4] += regs[1], regs[2] += regs[3];
        regs[2] ^= regs[3] <<  8, regs[5] += regs[2], regs[3] += regs[4];
        regs[3] ^= regs[4] >> 16, regs[6] += regs[3], regs[4] += regs[5];
        regs[4] ^= regs[5] << 10, regs[7] += regs[4], regs[5] += regs[6];
        regs[5] ^= regs[6] >>  4, regs[0] += regs[5], regs[6] += regs[7];
        regs[6] ^= regs[7] <<  8, regs[1] += regs[6], regs[7] += regs[0];
        regs[7] ^= regs[0] >>  9, regs[2] += regs[7], regs[0] += regs[1];
      };
    for(unsigned i = 0; i != 4; i += 1) {
      mix_regs();
    }
    for(unsigned i = 0; i != 256; i += 8) { ROCKET_ASSERT(this->m_mm[i] == 0); }
    auto scramble_round = [&](unsigned r)
      {
        for(unsigned k = 0; k != 8; k += 1) {
          regs[k] += this->m_mm[r + k];
        }
        mix_regs();
        for(unsigned k = 0; k != 8; k += 1) {
          this->m_mm[r + k] = regs[k];
        }
      };
    for(unsigned i = 0; i != 256; i += 8) {
      scramble_round(i);
    }
    for(unsigned i = 0; i != 256; i += 8) {
      scramble_round(i);
    }
    // Postpone generation of the first round to when a number is requested.
    this->m_ngot = 256;
  }

std::uint32_t Random_Number_Generator::bump() noexcept
  {
    unsigned index = this->m_ngot;
    if(index >= 256) {
      // Populate the pool with new numbers.
      this->do_update();
      index = 0;
    }
    // Advance the index and return the number at the previous position.
    this->m_ngot = static_cast<std::uint16_t>(index + 1);
    return this->m_pool[index];
  }

}  // namespace Asteria
