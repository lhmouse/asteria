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
        auto ptr = rocket::dynamic_pointer_cast<const TargetT>(
                     this->rocket::refcnt_base<Rcbase>::share_this<Rcbase>());
        if(!ptr) {
          this->do_throw_bad_cast(typeid(TargetT), typeid(*this));
        }
        return ptr;
      }
    template<typename TargetT> rcptr<TargetT> share_this()
      {
        auto ptr = rocket::dynamic_pointer_cast<TargetT>(
                     this->rocket::refcnt_base<Rcbase>::share_this<Rcbase>());
        if(!ptr) {
          this->do_throw_bad_cast(typeid(TargetT), typeid(*this));
        }
        return ptr;
      }
  };

}  // namespace Asteria

#endif
