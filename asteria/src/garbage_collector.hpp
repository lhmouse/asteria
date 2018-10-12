// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_GARBAGE_COLLECTOR_HPP_
#define ASTERIA_GARBAGE_COLLECTOR_HPP_

#include "fwd.hpp"

namespace Asteria {

class Garbage_collector
  {
  private:
    Vector<Rcptr<Variable>> m_vars;  // This is a flat map.

  public:
    Garbage_collector() noexcept
      : m_vars()
      {
      }
    ~Garbage_collector();

    Garbage_collector(const Garbage_collector &)
      = delete;
    Garbage_collector & operator=(const Garbage_collector &)
      = delete;

  public:
    bool track_variable(const Rcptr<Variable> &var);
    bool untrack_variable(const Rcptr<Variable> &var) noexcept;
    Size collect();
  };

}

#endif
