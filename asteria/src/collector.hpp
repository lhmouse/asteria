// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_COLLECTOR_HPP_
#define ASTERIA_COLLECTOR_HPP_

#include "fwd.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Collector
  {
  private:
    Collector *m_tied_opt;
    Vector<rocket::refcounted_ptr<Variable>> m_vars;  // This is a flat map.
    Bivector<rocket::refcounted_ptr<Variable>, long> m_gcrefs;  // This is a flat map.

  public:
    Collector() noexcept
      : m_tied_opt(nullptr), m_vars(), m_gcrefs()
      {
      }
    ~Collector();

    Collector(const Collector &)
      = delete;
    Collector & operator=(const Collector &)
      = delete;

  public:
    Collector * get_tied_collector_opt() const noexcept
      {
        return this->m_tied_opt;
      }
    void tie_collector(Collector *tied_opt) noexcept
      {
        this->m_tied_opt = tied_opt;
      }

    bool track_variable(const rocket::refcounted_ptr<Variable> &var);
    bool untrack_variable(const rocket::refcounted_ptr<Variable> &var) noexcept;
    void collect(bool unreserve);
  };

}

#endif
