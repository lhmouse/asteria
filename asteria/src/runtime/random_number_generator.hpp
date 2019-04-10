// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_RANDOM_NUMBER_GENERATOR_HPP_
#define ASTERIA_RUNTIME_RANDOM_NUMBER_GENERATOR_HPP_

#include "../fwd.hpp"
#include "rcbase.hpp"

namespace Asteria {

// This implements the ISAAC PRNG that is both very fast and cryptographically secure.
// The reference implementation assumes that `long` has 32 bits.
//   https://www.burtleburtle.net/bob/rand/isaac.html
class Random_Number_Generator : public Rcbase
  {
  private:
    // generator states
    std::uint32_t m_aa;
    std::uint32_t m_bb;
    std::uint32_t m_cc;
    std::uint32_t m_mm[256];
    // generated numbers
    std::uint32_t m_ngot;
    std::uint32_t m_pool[256];

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
    std::uint32_t bump() noexcept;
  };

}  // namespace Asteria

#endif
