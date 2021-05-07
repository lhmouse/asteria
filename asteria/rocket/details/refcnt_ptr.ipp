// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFCNT_PTR_HPP_
#  error Please include <rocket/refcnt_ptr.hpp> instead.
#endif

namespace details_refcnt_ptr {

class reference_counter_base
  {
  private:
    mutable reference_counter<long> m_nref;

  public:
    bool
    unique() const noexcept
      { return this->m_nref.unique();  }

    long
    use_count() const noexcept
      { return this->m_nref.get();  }

    void
    add_reference() const noexcept
      { return this->m_nref.increment();  }

    bool
    drop_reference() const noexcept
      { return this->m_nref.decrement();  }
  };

template<typename elementT, typename deleterT>
constexpr deleterT
copy_deleter(const refcnt_base<elementT, deleterT>& base) noexcept
  { return base.as_deleter();  }

template<typename elementT>
class stored_pointer
  {
  public:
    using element_type  = elementT;
    using pointer       = element_type*;

  private:
    pointer m_ptr = nullptr;

  public:
    constexpr
    stored_pointer() noexcept
      { }

    ~stored_pointer()
      {
        this->reset(nullptr);
#ifdef ROCKET_DEBUG
        if(is_trivially_destructible<pointer>::value)
          ::std::memset(static_cast<void*>(::std::addressof(this->m_ptr)), 0xF6, sizeof(m_ptr));
#endif
      }

    stored_pointer(const stored_pointer&)
      = delete;

    stored_pointer&
    operator=(const stored_pointer&)
      = delete;

  public:
    bool
    unique() const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return false;
        return ptr->reference_counter_base::unique();
      }

    long
    use_count() const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return 0;
        return ptr->reference_counter_base::use_count();
      }

    constexpr pointer
    get() const noexcept
      { return this->m_ptr;  }

    pointer
    release() noexcept
      { return ::std::exchange(this->m_ptr, nullptr);  }

    pointer
    fork() const noexcept
      {
        auto ptr = this->m_ptr;
        if(ptr)
          ptr->reference_counter_base::add_reference();
        return ptr;
      }

    ROCKET_FORCED_INLINE void
    reset(pointer ptr_new) noexcept
      {
        auto ptr = ::std::exchange(this->m_ptr, ptr_new);
        if(ptr)
          if(ROCKET_UNEXPECT(ptr->reference_counter_base::drop_reference()))
            (copy_deleter)(*ptr)(ptr);
      }

    void
    exchange_with(stored_pointer& other) noexcept
      { ::std::swap(this->m_ptr, other.m_ptr);  }
  };

template<typename targetT, typename sourceT, typename casterT>
refcnt_ptr<targetT>
pointer_cast_aux(const refcnt_ptr<sourceT>& sptr, casterT&& caster)
  {
    refcnt_ptr<targetT> dptr(::std::forward<casterT>(caster)(sptr.get()));
    if(dptr)
      dptr.get()->reference_counter_base::add_reference();
    return dptr;
  }

template<typename targetT, typename sourceT, typename casterT>
refcnt_ptr<targetT>
pointer_cast_aux(refcnt_ptr<sourceT>&& sptr, casterT&& caster)
  {
    refcnt_ptr<targetT> dptr(::std::forward<casterT>(caster)(sptr.get()));
    if(dptr)
      sptr.release();
    return dptr;
  }

}  // namespace details_refcnt_ptr
