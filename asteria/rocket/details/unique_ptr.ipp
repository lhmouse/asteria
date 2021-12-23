// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_PTR_HPP_
#  error Please include <rocket/unique_ptr.hpp> instead.
#endif

namespace details_unique_ptr {

template<typename elementT, typename deleterT, typename = void>
struct pointer_of
  : identity<elementT*>
  { };

template<typename elementT, typename deleterT>
struct pointer_of<elementT, deleterT, ROCKET_VOID_T(typename deleterT::pointer)>
  : identity<typename deleterT::pointer>
  { };

template<typename deleterT>
struct deleter_reference
  {
    typename remove_reference<deleterT>::type* m_qdel;

    constexpr
    deleter_reference(deleterT& del) noexcept
      : m_qdel(::std::addressof(del))
      { }

    constexpr operator
    deleterT&() const noexcept
      { return *(this->m_qdel);  }

    constexpr deleterT&
    get() const noexcept
      { return *(this->m_qdel);  }
  };

// deleter base wrapper
template<typename pointerT, typename deleterT,
         bool objT = is_object<deleterT>::value,
         bool refT = disjunction<is_function<deleterT>, is_reference<deleterT>>::value>
class stored_pointer;

// deleter of object types
template<typename pointerT, typename deleterT>
class stored_pointer<pointerT, deleterT, true, false>
  : private allocator_wrapper_base_for<deleterT>::type
  {
  public:
    using pointer       = pointerT;
    using deleter_type  = deleterT;

  private:
    using deleter_base = typename allocator_wrapper_base_for<deleterT>::type;

  private:
    pointer m_ptr;

  public:
    constexpr
    stored_pointer() noexcept(is_nothrow_constructible<deleter_type>::value)
      : deleter_base(),
        m_ptr()
      { }

    template<typename... dparamsT>
    explicit constexpr
    stored_pointer(pointer ptr, dparamsT&&... dparams)
      noexcept(is_nothrow_constructible<deleter_type, dparamsT&&...>::value)
      : deleter_base(::std::forward<dparamsT>(dparams)...),
        m_ptr(::std::move(ptr))
      { }

    ~stored_pointer()
      { this->reset(nullptr);  }

    stored_pointer(const stored_pointer&)
      = delete;

    stored_pointer&
    operator=(const stored_pointer&)
      = delete;

  public:
    const deleter_type&
    as_deleter() const noexcept
      { return static_cast<const deleter_base&>(*this);  }

    deleter_type&
    as_deleter() noexcept
      { return static_cast<deleter_base&>(*this);  }

    constexpr pointer
    get() const noexcept
      { return this->m_ptr;  }

    pointer
    release() noexcept
      { return ::std::exchange(this->m_ptr, nullptr);  }

    void
    reset(pointer ptr_new) noexcept
      {
        auto ptr = ::std::exchange(this->m_ptr, ::std::move(ptr_new));
        if(ptr)
          this->as_deleter()(ptr);
      }

    void
    exchange_with(stored_pointer& other) noexcept
      { ::std::swap(this->m_ptr, other.m_ptr);  }
  };

// deleter of reference types
template<typename pointerT, typename deleterT>
class stored_pointer<pointerT, deleterT, false, true>
  : private deleter_reference<deleterT>
  {
  public:
    using pointer       = pointerT;
    using deleter_type  = deleterT&;

  private:
    using deleter_base = deleter_reference<deleterT>;

  private:
    pointer m_ptr = nullptr;

  public:
    explicit constexpr
    stored_pointer(pointer ptr, deleterT& del) noexcept
      : deleter_base(del),
        m_ptr(::std::move(ptr))
      { }

    ~stored_pointer()
      { this->reset(nullptr);  }

    stored_pointer(const stored_pointer&)
      = delete;

    stored_pointer&
    operator=(const stored_pointer&)
      = delete;

  public:
    const deleter_base&
    as_deleter() const noexcept
      { return static_cast<const deleter_base&>(*this);  }

    deleter_base&
    as_deleter() noexcept
      { return static_cast<deleter_base&>(*this);  }

    constexpr pointer
    get() const noexcept
      { return this->m_ptr;  }

    pointer
    release() noexcept
      { return ::std::exchange(this->m_ptr, nullptr);  }

    void
    reset(pointer ptr_new) noexcept
      {
        auto ptr = ::std::exchange(this->m_ptr, ::std::move(ptr_new));
        if(ptr)
          this->as_deleter().get()(ptr);
      }

    void
    exchange_with(stored_pointer& other) noexcept
      { ::std::swap(this->m_ptr, other.m_ptr);  }
  };

template<typename targetT, typename sourceT, typename casterT>
unique_ptr<targetT>
pointer_cast_aux(unique_ptr<sourceT>&& uptr, casterT&& caster)
  {
    unique_ptr<targetT> dptr(::std::forward<casterT>(caster)(uptr.get()));
    if(dptr)
      uptr.release();
    return dptr;
  }

}  // namespace details_unique_ptr
