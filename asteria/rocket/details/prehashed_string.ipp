// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_PREHASHED_STRING_HPP_
#  error Please include <rocket/prehashed_string.hpp> instead.
#endif

namespace details_prehashed_string {

template<typename stringT, typename hashT, typename eqT>
class string_storage
  : private allocator_wrapper_base_for<hashT>::type,
    private allocator_wrapper_base_for<eqT>::type
  {
  public:
    using string_type  = stringT;
    using hasher       = hashT;
    using key_equal    = eqT;

  private:
    using hasher_base     = typename allocator_wrapper_base_for<hasher>::type;
    using key_equal_base  = typename allocator_wrapper_base_for<key_equal>::type;

  private:
    string_type m_str;
    size_t m_hval;

  public:
    template<typename... paramsT>
    explicit constexpr
    string_storage(const hasher& hf, const key_equal& eq, paramsT&&... params)
      : hasher_base(hf),
        key_equal_base(eq),
        m_str(::std::forward<paramsT>(params)...),
        m_hval(this->as_hasher()(this->m_str))
      { }

    string_storage(const string_storage&)
      = delete;

    string_storage&
    operator=(const string_storage&)
      = delete;

  public:
    constexpr const hasher&
    as_hasher() const noexcept
      { return static_cast<const hasher_base&>(*this);  }

    constexpr hasher&
    as_hasher() noexcept
      { return static_cast<hasher_base&>(*this);  }

    constexpr const key_equal&
    as_key_equal() const noexcept
      { return static_cast<const key_equal_base&>(*this);  }

    constexpr key_equal&
    as_key_equal() noexcept
      { return static_cast<key_equal_base&>(*this);  }

    constexpr const string_type&
    str() const noexcept
      { return this->m_str;  }

    constexpr size_t
    hval() const noexcept
      { return this->m_hval;  }

    void
    clear()
      {
        this->m_str.clear();
        this->m_hval = this->as_hasher()(this->m_str);
      }

    template<typename... paramsT>
    void
    assign(paramsT&&... params)
      {
        this->m_str.assign(::std::forward<paramsT>(params)...);
        this->m_hval = this->as_hasher()(this->m_str);
      }

    template<typename paramT>
    void
    set_string(paramT&& param)
      {
        this->m_str = ::std::forward<paramT>(param);
        this->m_hval = this->as_hasher()(this->m_str);
      }

    void
    share_with(const string_storage& other)
      {
        this->m_str = other.m_str;
        this->m_hval = other.m_hval;
      }

    void
    exchange_with(string_storage& other)
      {
        noadl::xswap(this->m_str, other.m_str);
        noadl::xswap(this->m_hval, other.m_hval);
      }
  };

}  // namespace details_prehashed_string
