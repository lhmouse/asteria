// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_PTR_
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
    typename remove_reference<deleterT>::type* m_del;

    constexpr
    deleter_reference(deleterT& del) noexcept
      : m_del(::std::addressof(del))  { }

    constexpr operator
    deleterT&() const noexcept
      { return *(this->m_del);  }

    constexpr
    deleterT&
    get() const noexcept
      { return *(this->m_del);  }
  };

// deleter base wrapper
template<typename pointerT, typename deleterT, bool ptrT, bool objT, bool refT>
class stored_pointer_impl;

// deleter of pointer types
template<typename pointerT, typename deleterT>
class stored_pointer_impl<pointerT, deleterT, true, true, false>
  {
  public:
    using pointer       = pointerT;
    using deleter_type  = deleterT;

  private:
    deleter_type m_del;
    pointer m_ptr;

  public:
    constexpr
    stored_pointer_impl() noexcept
      : m_del(), m_ptr()  { }

    explicit constexpr
    stored_pointer_impl(pointer ptr, deleter_type del = nullptr) noexcept
      : m_del(del), m_ptr(::std::move(ptr))  { }

    ~stored_pointer_impl()
      { this->reset(nullptr);  }

    stored_pointer_impl(const stored_pointer_impl&) = delete;

    stored_pointer_impl&
    operator=(const stored_pointer_impl&) = delete;

  public:
    const deleter_type&
    as_deleter() const noexcept
      { return this->m_del;  }

    deleter_type&
    as_deleter() noexcept
      { return this->m_del;  }

    constexpr
    pointer
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
          (*(this->as_deleter()))(ptr);
      }

    void
    exchange_with(stored_pointer_impl& other) noexcept
      { ::std::swap(this->m_ptr, other.m_ptr);  }
  };

// deleter of non-pointer object types
template<typename pointerT, typename deleterT>
class stored_pointer_impl<pointerT, deleterT, false, true, false>
  : private allocator_wrapper_base_for<deleterT>::type
  {
  public:
    using pointer       = pointerT;
    using deleter_type  = deleterT;

  private:
    using deleter_base = typename allocator_wrapper_base_for<deleterT>::type;
    pointer m_ptr;

  public:
    constexpr
    stored_pointer_impl() noexcept(is_nothrow_constructible<deleter_type>::value)
      : deleter_base(), m_ptr()  { }

    template<typename... dparamsT>
    explicit constexpr
    stored_pointer_impl(pointer ptr, dparamsT&&... dparams)
      noexcept(is_nothrow_constructible<deleter_type, dparamsT&&...>::value)
      : deleter_base(::std::forward<dparamsT>(dparams)...), m_ptr(::std::move(ptr))  { }

    ~stored_pointer_impl()
      { this->reset(nullptr);  }

    stored_pointer_impl(const stored_pointer_impl&) = delete;

    stored_pointer_impl&
    operator=(const stored_pointer_impl&) = delete;

  public:
    const deleter_type&
    as_deleter() const noexcept
      { return static_cast<const deleter_base&>(*this);  }

    deleter_type&
    as_deleter() noexcept
      { return static_cast<deleter_base&>(*this);  }

    constexpr
    pointer
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
    exchange_with(stored_pointer_impl& other) noexcept
      { ::std::swap(this->m_ptr, other.m_ptr);  }
  };

// deleter of reference types
template<typename pointerT, typename deleterT>
class stored_pointer_impl<pointerT, deleterT, false, false, true>
  : private deleter_reference<deleterT>
  {
  public:
    using pointer       = pointerT;
    using deleter_type  = deleterT&;

  private:
    using deleter_base = deleter_reference<deleterT>;
    pointer m_ptr = nullptr;

  public:
    explicit constexpr
    stored_pointer_impl(pointer ptr, deleterT& del) noexcept
      : deleter_base(del), m_ptr(::std::move(ptr))  { }

    ~stored_pointer_impl()
      { this->reset(nullptr);  }

    stored_pointer_impl(const stored_pointer_impl&) = delete;

    stored_pointer_impl&
    operator=(const stored_pointer_impl&) = delete;

  public:
    const deleter_base&
    as_deleter() const noexcept
      { return static_cast<const deleter_base&>(*this);  }

    deleter_base&
    as_deleter() noexcept
      { return static_cast<deleter_base&>(*this);  }

    constexpr
    pointer
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
    exchange_with(stored_pointer_impl& other) noexcept
      { ::std::swap(this->m_ptr, other.m_ptr);  }
  };

template<typename pointerT, typename deleterT>
using stored_pointer
  = stored_pointer_impl<pointerT, deleterT,
        is_pointer<deleterT>::value, is_object<deleterT>::value,
        is_function<deleterT>::value || is_reference<deleterT>::value>;

}  // namespace details_unique_ptr
