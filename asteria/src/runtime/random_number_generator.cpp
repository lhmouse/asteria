// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "random_number_generator.hpp"
#include "../utilities.hpp"
#include <fcntl.h>  // ::open()
#include <unistd.h>  // ::read()

namespace Asteria {

Random_Number_Generator::~Random_Number_Generator()
  {
  }

    namespace {

    constexpr uint32_t do_sll(uint32_t reg, int bits) noexcept
      {
        return reg << bits;
      }

    constexpr uint32_t do_srl(uint32_t reg, int bits) noexcept
      {
        return reg >> bits;
      }

    }  // namespace

void Random_Number_Generator::do_update() noexcept
  {
    // Increment `cc` and combine it with `bb` for every round.
    this->m_cc += 1;
    this->m_bb += this->m_cc;
    // Unroll the loop by 4.
    for(size_t i = 0; i < 64; ++i) {
      auto step = [&](size_t r, auto shift, int b)
        {
          auto x = this->m_mm[r];
          auto y = this->m_aa;
          y ^= shift(y, b);
          y += this->m_mm[(r+128)%256];
          this->m_aa = y;
          y += this->m_mm[(x>>2)%256] + this->m_bb;
          this->m_mm[r] = y;
          x += this->m_mm[(y>>10)%256];
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

    namespace {

    bool do_read_random_device(void* data, size_t size) noexcept
      {
        std::memset(data, 'x', size);
        int fd = ::open("/dev/urandom", O_RDONLY);
        if(fd == -1) {
          return false;
        }
        auto nread = ::read(fd, data, size);
        ::close(fd);
        return nread > 0;
      }

    class Scrambler
      {
      private:
        uint32_t m_regs[8];

      public:
        Scrambler() noexcept
          {
            for(size_t i = 0; i < 8; ++i) {
              this->m_regs[i] = 0x9E3779B9;
            }
            for(size_t i = 0; i < 4; ++i) {
              this->mix();
            }
          }

      public:
        void combine(const uint32_t* src) noexcept
          {
            for(size_t i = 0; i < 8; ++i) {
              this->m_regs[i] += src[i];
            }
          }
        void mix() noexcept
          {
            auto step = [&](size_t r, auto shift, int b)
              {
                this->m_regs[r] ^= shift(this->m_regs[(r+1)%8], b);
                this->m_regs[(r+3)%8] += this->m_regs[r];
                this->m_regs[(r+1)%8] += this->m_regs[(r+2)%8];
              };
            step(0, do_sll, 11);
            step(1, do_srl,  2);
            step(2, do_sll,  8);
            step(3, do_srl, 16);
            step(4, do_sll, 10);
            step(5, do_srl,  4);
            step(6, do_sll,  8);
            step(7, do_srl,  9);
          }
        void output(uint32_t* out) const noexcept
          {
            for(size_t i = 0; i < 8; ++i) {
              out[i] = this->m_regs[i];
            }
          }
      };

    }  // namespace

void Random_Number_Generator::reset() noexcept
  {
    // Initialize internal state data.
    this->m_aa = 0;
    this->m_bb = 0;
    this->m_cc = 0;
    do_read_random_device(this->m_mm, sizeof(m_mm));
    // Scramble words.
    Scrambler regs;
    for(size_t i = 0; i < 64; ++i) {
      auto view = this->m_mm + i%32*8;
      regs.combine(view);
      regs.mix();
      regs.output(view);
    }
    // Discard the result of the very first round.
    this->do_update();
    // Postpone generation of the next round to when a number is requested.
    this->m_ngot = 256;
  }

uint32_t Random_Number_Generator::bump() noexcept
  {
    auto index = this->m_ngot;
    if(index >= 256) {
      // Populate the pool with new numbers.
      this->do_update();
      index = 0;
    }
    // Advance the index and return the number at the previous position.
    this->m_ngot = index + 1;
    return this->m_pool[index];
  }

}  // namespace Asteria
