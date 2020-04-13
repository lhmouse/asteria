// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_PREHASHED_STRING_HPP_
#  error Please include <rocket/prehashed_string.hpp> instead.
#endif

namespace details_prehashed_string {

template<typename stringT, typename hashT>
class string_storage
  : private allocator_wrapper_base_for<hashT>::type
  {
  public:
    using string_type  = stringT;
    using hasher       = hashT;

  private:
    using hasher_base   = typename allocator_wrapper_base_for<hasher>::type;

  private:
    string_type m_str;
    size_t m_hval;

  public:
    template<typename... paramsT>
    explicit
    string_storage(const hasher& hf, paramsT&&... params)
      : hasher_base(hf), m_str(::std::forward<paramsT>(params)...), m_hval(this->as_hasher()(this->m_str))
      { }

    string_storage(const string_storage&)
      = delete;

    string_storage&
    operator=(const string_storage&)
      = delete;

  public:
    const hasher&
    as_hasher()
    const noexcept
      { return static_cast<const hasher_base&>(*this);  }

    hasher&
    as_hasher()
    noexcept
      { return static_cast<hasher_base&>(*this);  }

    const string_type&
    str()
    const noexcept
      { return this->m_str;  }

    size_t
    hval()
    const noexcept
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

    void
    assign(const string_storage& other)
      {
        this->m_str = other.m_str;
        this->m_hval = other.m_hval;
      }

    void
    assign(string_storage&& other)
      {
        this->m_str = ::std::move(other.m_str);
        this->m_hval = this->as_hasher()(this->m_str);
      }

    void
    exchange_with(string_storage& other)
      {
        noadl::xswap(this->m_str, other.m_str);
        noadl::xswap(this->m_hval, other.m_hval);
      }
  };

}  // namespace details_prehashed_string
