// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_REFCNT_PTR_
#  error Please include <rocket/refcnt_ptr.hpp> instead.
#endif
namespace details_refcnt_ptr {

class refcnt_cJveMKH5bI7L
  {
  private:
    mutable reference_counter<int> m_nref;

  public:
    bool
    unique()
      const noexcept
      { return this->m_nref.unique();  }

    int
    use_count()
      const noexcept
      { return this->m_nref.get();  }

    int
    add_reference()
      const noexcept
      { return this->m_nref.increment();  }

    int
    drop_reference()
      const noexcept
      { return this->m_nref.decrement();  }
  };

template<typename elementT, typename deleterT>
constexpr
deleterT
move_deleter(const refcnt_base<elementT, deleterT>& base)
  noexcept
  {
    return base.as_deleter();
  }

template<typename elementT, typename deleterT>
constexpr
deleterT
move_deleter(refcnt_base<elementT, deleterT>& base)
  noexcept
  {
    return move(base.as_deleter());
  }

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
    stored_pointer()
      noexcept
      :
        m_ptr()
      { }

    explicit constexpr
    stored_pointer(pointer ptr)
      noexcept
      :
        m_ptr(move(ptr))
      { }

    ~stored_pointer()
      { this->reset(nullptr);  }

    stored_pointer(const stored_pointer&) = delete;
    stored_pointer& operator=(const stored_pointer&) = delete;

  public:
    bool
    unique()
      const noexcept
      { return this->m_ptr && this->m_ptr->refcnt_cJveMKH5bI7L::unique();  }

    int
    use_count()
      const noexcept
      { return this->m_ptr ? this->m_ptr->refcnt_cJveMKH5bI7L::use_count() : 0;  }

    constexpr
    pointer
    get()
      const noexcept
      { return this->m_ptr;  }

    pointer
    release()
      noexcept
      {
        if(this->m_ptr == nullptr)
          return nullptr;

        auto ptr_old = noadl::exchange(this->m_ptr);
        return ptr_old;
      }

    pointer
    fork()
      const noexcept
      {
        if(this->m_ptr == nullptr)
          return nullptr;

        this->m_ptr->refcnt_cJveMKH5bI7L::add_reference();
        return this->m_ptr;
      }

    void
    reset(pointer ptr_new)
      noexcept
      {
        if((this->m_ptr == nullptr) && (ptr_new == nullptr))
          return;

        auto ptr_old = noadl::exchange(this->m_ptr, ptr_new);
        if((ptr_old != nullptr) && (ptr_old->refcnt_cJveMKH5bI7L::drop_reference() == 0))
          noadl::details_refcnt_ptr::move_deleter(*ptr_old) (ptr_old);
      }

    void
    exchange_with(stored_pointer& other)
      noexcept
      { ::std::swap(this->m_ptr, other.m_ptr);  }
  };

}  // namespace details_refcnt_ptr
