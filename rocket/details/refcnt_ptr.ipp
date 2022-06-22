// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFCNT_PTR_
#  error Please include <rocket/refcnt_ptr.hpp> instead.
#endif

namespace details_refcnt_ptr {

class refcnt_cJveMKH5bI7L
  {
  private:
    mutable reference_counter<long> m_nref;

  public:
    bool
    unique() const noexcept
      { return this->m_nref == 1;  }

    long
    use_count() const noexcept
      { return this->m_nref;  }

    long
    add_reference() const noexcept
      { return ++(this->m_nref);  }

    long
    drop_reference() const noexcept
      { return --(this->m_nref);  }
  };

template<typename elementT, typename deleterT>
constexpr
deleterT
copy_deleter(const refcnt_base<elementT, deleterT>& base) noexcept
  { return base.as_deleter();  }

template<typename elementT>
class stored_pointer
  {
  public:
    using element_type  = elementT;
    using pointer       = elementT*;

  private:
    pointer m_ptr;

  public:
    constexpr
    stored_pointer() noexcept
      : m_ptr()
      { }

    explicit constexpr
    stored_pointer(pointer ptr) noexcept
      : m_ptr(::std::move(ptr))
      { }

    ~stored_pointer()
      { this->reset(nullptr);  }

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
        return ptr->refcnt_cJveMKH5bI7L::unique();
      }

    long
    use_count() const noexcept
      {
        auto ptr = this->m_ptr;
        if(!ptr)
          return 0;
        return ptr->refcnt_cJveMKH5bI7L::use_count();
      }

    constexpr
    pointer
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
          ptr->refcnt_cJveMKH5bI7L::add_reference();
        return ptr;
      }

    ROCKET_ALWAYS_INLINE
    void
    reset(pointer ptr_new) noexcept
      {
        auto ptr = ::std::exchange(this->m_ptr, ptr_new);
        if(ptr)
          if(ROCKET_UNEXPECT(ptr->refcnt_cJveMKH5bI7L::drop_reference() == 0))
            (copy_deleter)(*ptr)(ptr);
      }

    void
    exchange_with(stored_pointer& other) noexcept
      { ::std::swap(this->m_ptr, other.m_ptr);  }
  };

}  // namespace details_refcnt_ptr
