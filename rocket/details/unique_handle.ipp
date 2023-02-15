// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_HANDLE_
#  error Please include <rocket/unique_handle.hpp> instead.
#endif
namespace details_unique_handle {

template<typename handleT, typename closerT>
class stored_handle
  : private allocator_wrapper_base_for<closerT>::type
  {
  public:
    using handle_type  = handleT;
    using closer_type  = closerT;

  private:
    using closer_base = typename allocator_wrapper_base_for<closerT>::type;

  private:
    handle_type m_hv;

  public:
    explicit constexpr
    stored_handle() noexcept(is_nothrow_constructible<closer_type>::value)
      : closer_base(),
        m_hv(this->as_closer().null())  { }

    template<typename... clparamsT>
    explicit constexpr
    stored_handle(handle_type hv, clparamsT&&... clparams)
      noexcept(is_nothrow_constructible<closer_type, clparamsT&&...>::value)
      : closer_base(::std::forward<clparamsT>(clparams)...),
        m_hv(::std::move(hv))  { }

    ~stored_handle()
      { this->reset(this->as_closer().null());  }

    stored_handle(const stored_handle&) = delete;

    stored_handle&
    operator=(const stored_handle&) = delete;

  public:
    const closer_type&
    as_closer() const noexcept
      { return static_cast<const closer_base&>(*this);  }

    closer_type&
    as_closer() noexcept
      { return static_cast<closer_base&>(*this);  }

    constexpr
    const handle_type&
    get() const noexcept
      { return this->m_hv;  }

    handle_type
    release() noexcept
      { return ::std::exchange(this->m_hv, this->as_closer().null());  }

    void
    reset(handle_type hv_new) noexcept
      {
        auto hv_old = ::std::exchange(this->m_hv, ::std::move(hv_new));
        if(!this->as_closer().is_null(hv_old))
          this->as_closer().close(::std::move(hv_old));
      }

    void
    exchange_with(stored_handle& other) noexcept
      { ::std::swap(this->m_hv, other.m_hv);  }
  };

template<typename handleT, typename closerT>
class default_closer_wrapper
  {
  private:
    typename conditional<
        is_function<typename remove_reference<closerT>::type>::value,
        closerT&,                      // take functions by reference
        typename decay<closerT>::type  // take non-functions by value
      >::type m_cl;

  public:
    constexpr  // non-explicit
    default_closer_wrapper(closerT&& xcl)
      : m_cl(::std::forward<closerT>(xcl))  { }

  public:
    constexpr
    handleT
    null() const noexcept
      { return { };  }

    constexpr
    bool
    is_null(handleT hv) const noexcept
      { return static_cast<bool>(hv) == false;  }

    constexpr
    void
    close(handleT hv)
      { this->m_cl(hv);  }
  };

}  // namespace details_unique_handle
