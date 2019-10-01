// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_RCBASE_HPP_
#define ASTERIA_RCBASE_HPP_

#include "fwd.hpp"

namespace Asteria {

class Rcbase : public rocket::refcnt_base<Rcbase>
  {
  public:
    Rcbase() noexcept
      {
      }
    Rcbase(const Rcbase&) noexcept
      : Rcbase()
      {
      }
    Rcbase& operator=(const Rcbase&) noexcept
      {
        return *this;
      }
    virtual ~Rcbase();

  public:
    bool unique() const noexcept
      {
        return rocket::refcnt_base<Rcbase>::unique();
      }
    long use_count() const noexcept
      {
        return rocket::refcnt_base<Rcbase>::use_count();
      }

    template<typename TargetT> rcptr<const TargetT> share_this() const
      {
        return rocket::refcnt_base<Rcbase>::share_this<TargetT>();
      }
    template<typename TargetT> rcptr<TargetT> share_this()
      {
        return rocket::refcnt_base<Rcbase>::share_this<TargetT>();
      }
  };

}  // namespace Asteria

#endif
