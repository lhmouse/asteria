// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_GARBAGE_COLLECTOR_HPP_
#define ASTERIA_GARBAGE_COLLECTOR_HPP_

#include "fwd.hpp"
#include "rocket/refcounted_ptr.hpp"

namespace Asteria {

class Garbage_collector : public rocket::refcounted_base<Garbage_collector>
  {
  private:
    Garbage_collector *m_tied_opt;
    Vector<rocket::refcounted_ptr<Variable>> m_vars;  // This is a flat map.
    Bivector<rocket::refcounted_ptr<Variable>, long> m_gcrefs;  // This is a flat map.

  public:
    Garbage_collector() noexcept
      : m_tied_opt(nullptr), m_vars(), m_gcrefs()
      {
      }
    ~Garbage_collector();

    Garbage_collector(const Garbage_collector &)
      = delete;
    Garbage_collector & operator=(const Garbage_collector &)
      = delete;

  public:
    Garbage_collector * get_tied_collector_opt() const noexcept
      {
        return this->m_tied_opt;
      }
    void tie_collector(Garbage_collector *tied_opt) noexcept
      {
        this->m_tied_opt = tied_opt;
      }

    bool track_variable(const rocket::refcounted_ptr<Variable> &var);
    bool untrack_variable(const rocket::refcounted_ptr<Variable> &var) noexcept;
    void collect(bool unreserve);
  };

}

#endif
