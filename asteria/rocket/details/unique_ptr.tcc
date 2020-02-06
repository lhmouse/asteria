// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UNIQUE_PTR_HPP_
#  error Please include <rocket/unique_ptr.hpp> instead.
#endif

namespace details_unique_ptr {

template<typename elementT, typename deleterT, typename = void>
    struct pointer_of : enable_if<1, elementT*>
  {
  };
template<typename elementT, typename deleterT>
    struct pointer_of<elementT, deleterT,
                      typename make_void<typename deleterT::pointer>::type> : enable_if<1, typename deleterT::pointer>
  {
  };

template<typename pointerT, typename deleterT>
    class stored_pointer : private allocator_wrapper_base_for<deleterT>::type
  {
  public:
    using pointer       = pointerT;
    using deleter_type  = deleterT;

  private:
    using deleter_base = typename allocator_wrapper_base_for<deleterT>::type;

  private:
    pointer m_ptr;

  public:
    constexpr stored_pointer() noexcept(is_nothrow_constructible<deleter_type>::value)
      :
        deleter_base(),
        m_ptr()
      {
      }
    explicit constexpr stored_pointer(const deleter_type& del) noexcept
      :
        deleter_base(del),
        m_ptr()
      {
      }
    explicit constexpr stored_pointer(deleter_type&& del) noexcept
      :
        deleter_base(noadl::move(del)),
        m_ptr()
      {
      }
    ~stored_pointer()
      {
        this->reset(pointer());
#ifdef ROCKET_DEBUG
        if(is_trivially_destructible<pointer>::value)
          ::std::memset(static_cast<void*>(::std::addressof(this->m_ptr)), 0xF6, sizeof(m_ptr));
#endif
      }

    stored_pointer(const stored_pointer&)
      = delete;
    stored_pointer& operator=(const stored_pointer&)
      = delete;

  public:
    const deleter_type& as_deleter() const noexcept
      {
        return static_cast<const deleter_base&>(*this);
      }
    deleter_type& as_deleter() noexcept
      {
        return static_cast<deleter_base&>(*this);
      }

    constexpr pointer get() const noexcept
      {
        return this->m_ptr;
      }
    pointer release() noexcept
      {
        return ::std::exchange(this->m_ptr, pointer());
      }
    void reset(pointer ptr_new) noexcept
      {
        auto ptr = ::std::exchange(this->m_ptr, noadl::move(ptr_new));
        if(ptr)
          this->as_deleter()(ptr);
      }
    void exchange_with(stored_pointer& other) noexcept
      {
        xswap(this->m_ptr, other.m_ptr);
      }
  };

template<typename targetT, typename sourceT, typename casterT>
    unique_ptr<targetT> pointer_cast_aux(unique_ptr<sourceT>&& uptr, casterT&& caster)
  {
    unique_ptr<targetT> dptr(noadl::forward<casterT>(caster)(uptr.get()));
    if(dptr)
      uptr.release();
    return dptr;
  }

}  // namespace details_unique_ptr
