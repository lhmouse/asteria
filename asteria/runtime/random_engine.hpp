// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_RANDOM_NUMBER_GENERATOR_
#define ASTERIA_RUNTIME_RANDOM_NUMBER_GENERATOR_

#include "../fwd.hpp"
namespace asteria {

class Random_Engine final
  : public rcfwd<Random_Engine>
  {
  public:
    // This implements the ISAAC PRNG that is cryptographically secure.
    // The reference implementation assumes that `long` has 32 bits.
    //   https://www.burtleburtle.net/bob/rand/isaac.html
    using result_type  = uint32_t;

  private:
    // This matches `struct randctx` from 'rand.h'.
    //   https://www.burtleburtle.net/bob/c/rand.h
    uint32_t m_randcnt;
    uint32_t m_randrsl[256];
    uint32_t m_randmem[256];
    uint32_t m_randa;
    uint32_t m_randb;
    uint32_t m_randc;

  public:
    explicit
    Random_Engine() noexcept
      { this->init();  }

  private:
    void
    do_isaac() noexcept;

  public:
    ASTERIA_COPYABLE_DESTRUCTOR(Random_Engine);

    // Initializes this PRNG with some external entropy source.
    void
    init() noexcept;

    // Gets a random 32-bit number.
    uint32_t
    bump() noexcept
      {
        // This matches `main()` from 'rand.c'.
        //   https://www.burtleburtle.net/bob/c/rand.c
        uint32_t off = this->m_randcnt++ % 256;
        if(ROCKET_UNEXPECT(off == 0))
          this->do_isaac();
        return this->m_randrsl[off];
      }

    // This class is a UniformRandomBitGenerator.
    static constexpr
    result_type
    min() noexcept
      { return 0; }

    static constexpr
    result_type
    max() noexcept
      { return UINT32_MAX;  }

    result_type
    operator()() noexcept
      { return this->bump();  }
  };

}  // namespace asteria
#endif
