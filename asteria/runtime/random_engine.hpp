// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_RANDOM_NUMBER_GENERATOR_
#define ASTERIA_RUNTIME_RANDOM_NUMBER_GENERATOR_

#include "../fwd.hpp"
namespace asteria {

class Random_Engine
  :
    public rcfwd<Random_Engine>
  {
  public:
    // This implements ISAAC (indirection, shift, accumulate, add, and count),
    // a cryptographically secure pseudorandom number generator, designed by
    // Robert J. Jenkins Jr. in 1993. The reference implementation assumes that
    // `long` is a 32-bit type.
    // Reference: https://www.burtleburtle.net/bob/rand/isaac.html
    using result_type  = uint32_t;

  private:
    // https://www.burtleburtle.net/bob/c/rand.h
    struct randctx
      {
        uint32_t randcnt;
        uint32_t randrsl[256];
        uint32_t randmem[256];
        uint32_t randa;
        uint32_t randb;
        uint32_t randc;
      };

    union {
      unsigned char m_ctx_init[sizeof(randctx)];
      randctx m_ctx;
    };

  public:
    // Creates a PRNG from some external entropy source. It is not necessary
    // to seed this generator explicitly.
    Random_Engine() noexcept;

  private:
    uint32_t
    do_isaac() noexcept;

  public:
    Random_Engine(const Random_Engine&) = delete;
    Random_Engine& operator=(const Random_Engine&) & = delete;
    ~Random_Engine();

    // Gets a random 32-bit number.
    uint32_t
    bump() noexcept
      {
        return ROCKET_EXPECT(this->m_ctx.randcnt != 0)
               ? *(this->m_ctx.randrsl + (-- this->m_ctx.randcnt))
               : this->do_isaac();
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
