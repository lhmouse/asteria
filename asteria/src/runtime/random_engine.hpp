// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_RANDOM_NUMBER_GENERATOR_HPP_
#define ASTERIA_RUNTIME_RANDOM_NUMBER_GENERATOR_HPP_

#include "../fwd.hpp"

namespace Asteria {

class Random_Engine
final
  :public Rcfwd<Random_Engine>
  {
  public:
    using result_type  = uint32_t;

    static
    constexpr
    result_type
    min()
    noexcept
      { return 0; }

    static
    constexpr
    result_type
    max()
    noexcept
      { return UINT32_MAX;  }

  private:
    // This implements the ISAAC PRNG that is both very fast and
    // cryptographically secure.
    // The reference implementation assumes that `long` has 32 bits.
    //   https://www.burtleburtle.net/bob/rand/isaac.html

    // output pool
    uint32_t m_ngot;
    uint32_t m_pool[256];

    // internal states
    uint32_t m_aa;
    uint32_t m_bb;
    uint32_t m_cc;
    uint32_t m_mm[256];

  public:
    Random_Engine()
    noexcept
      { this->init();  }

    ~Random_Engine()
    override;

    ASTERIA_DECLARE_NONCOPYABLE(Random_Engine);

  private:
    inline
    void
    do_clear()
    noexcept;

    inline
    void
    do_update()
    noexcept;

  public:
    // Initialize this PRNG with some external entropy source.
    void
    init()
    noexcept;

    // Get a random 32-bit number.
    uint32_t
    bump()
    noexcept;

    result_type
    operator()()
    noexcept
      { return this->bump();  }
  };

}  // namespace Asteria

#endif
