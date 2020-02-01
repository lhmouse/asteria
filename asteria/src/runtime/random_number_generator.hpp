// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_RANDOM_NUMBER_GENERATOR_HPP_
#define ASTERIA_RUNTIME_RANDOM_NUMBER_GENERATOR_HPP_

#include "../fwd.hpp"
#include "../rcbase.hpp"

namespace Asteria {

class Random_Number_Generator final : public virtual Rcbase
  {
  private:
    // This implements the ISAAC PRNG that is both very fast and cryptographically secure.
    // The reference implementation assumes that `long` has 32 bits.
    //   https://www.burtleburtle.net/bob/rand/isaac.html

    // output pool
    uint32_t m_ngot;
    uint32_t m_pool[256];
    // internal state
    uint32_t m_aa;
    uint32_t m_bb;
    uint32_t m_cc;
    uint32_t m_mm[256];

  public:
    Random_Number_Generator() noexcept
      {
        this->reset();
      }
    ~Random_Number_Generator() override;

    Random_Number_Generator(const Random_Number_Generator&)
      = delete;
    Random_Number_Generator& operator=(const Random_Number_Generator&)
      = delete;

  private:
    inline void do_clear() noexcept;
    inline void do_update() noexcept;

  public:
    void reset() noexcept;
    uint32_t bump() noexcept;
  };

}  // namespace Asteria

#endif
