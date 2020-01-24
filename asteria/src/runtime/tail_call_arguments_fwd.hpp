// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RUNTIME_TAIL_CALL_ARGUMENTS_FWD_HPP_
#define ASTERIA_RUNTIME_TAIL_CALL_ARGUMENTS_FWD_HPP_

#include "../fwd.hpp"
#include "../rcbase.hpp"

namespace Asteria {

class Tail_Call_Arguments_Fwd : public virtual Rcbase
  {
    friend class Tail_Call_Arguments;

  private:
    Tail_Call_Arguments_Fwd() noexcept
      {
      }

  public:
    ~Tail_Call_Arguments_Fwd() override;

    Tail_Call_Arguments_Fwd(const Tail_Call_Arguments_Fwd&)
      = delete;
    Tail_Call_Arguments_Fwd& operator=(const Tail_Call_Arguments_Fwd&)
      = delete;
  };

}  // namespace Asteria

#endif
