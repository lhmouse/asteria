// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_STATIC_VECTOR_
#  error Please include <rocket/static_vector.hpp> instead.
#endif
namespace details_static_vector {

template<typename allocT, size_t capacityT>
class storage_handle
  :
    private allocator_wrapper_base_for<allocT>::type
  {
  public:
    using allocator_type   = allocT;
    using value_type       = typename allocator_type::value_type;
    using size_type        = typename allocator_traits<allocator_type>::size_type;

  private:
    using allocator_base  = typename allocator_wrapper_base_for<allocator_type>::type;
    using nelem_type      = typename lowest_unsigned<capacityT - 1>::type;

  private:
    static constexpr size_t my_align = alignof(value_type);

    union {
      nelem_type m_nelem;

      // This eliminates padding bytes for constexpr initialization.
      typename conditional<(my_align < 4),
            conditional<(my_align == 1),  uint8_t, uint16_t>,  // 1, 2
            conditional<(my_align == 4), uint32_t, uint64_t>   // 4, 8
          >::type::type m_init_nelem;
    };

    union {
      value_type m_data[capacityT];

      // This suppresses warnings about potential use of uninitialized objects.
      volatile char m_do_not_use;
    };

  public:
    constexpr
    storage_handle()
      noexcept(is_nothrow_constructible<allocator_type>::value)
      :
        allocator_base(), m_init_nelem(0)
      { }

    explicit
    storage_handle(const allocator_type& alloc)
      noexcept
      :
        allocator_base(alloc), m_init_nelem(0)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this->m_data), '*', sizeof(m_data));
#endif
      }

    explicit
    storage_handle(allocator_type&& alloc)
      noexcept
      :
        allocator_base(move(alloc)), m_init_nelem(0)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this->m_data), '*', sizeof(m_data));
#endif
      }

#ifdef __cpp_constexpr_dynamic_alloc
    constexpr
#endif
    ~storage_handle()
      {
        // Destroy all elements backwards.
        size_t off = this->m_nelem;
        while(off-- != 0)
          allocator_traits<allocator_type>::destroy(*this, this->m_data + off);

#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this->m_data), '~', sizeof(m_data));
        this->m_nelem = static_cast<nelem_type>(0xBAD1BEEF);
#endif
      }

    storage_handle(const storage_handle&) = delete;
    storage_handle& operator=(const storage_handle&) = delete;

  public:
    constexpr
    const allocator_type&
    as_allocator()
      const noexcept
      { return static_cast<const allocator_base&>(*this);  }

    allocator_type&
    as_allocator()
      noexcept
      { return static_cast<allocator_base&>(*this);  }

    ROCKET_CONST static constexpr
    size_type
    capacity()
      noexcept
      { return capacityT;  }

    constexpr
    size_type
    max_size()
      const noexcept
      { return this->capacity();  }

    size_type
    check_size_add(size_type base, size_type add)
      const
      {
        bool ovr = false;
        size_type res = noadl::addm(base, add, &ovr);
        if(ovr || (res > this->max_size()))
          noadl::sprintf_and_throw<length_error>(
              "static_vector: max size exceeded (`%lld` + `%lld` > `%lld`)",
              static_cast<long long>(base), static_cast<long long>(add),
              static_cast<long long>(this->max_size()));
        return res;
      }

    ROCKET_PURE constexpr
    const value_type*
    data()
      const noexcept
      { return this->m_data;  }

    value_type*
    mut_data()
      noexcept
      { return this->m_data;  }

    ROCKET_PURE constexpr
    size_type
    size()
      const noexcept
      { return this->m_nelem;  }

    template<typename... paramsT>
    value_type&
    emplace_back_unchecked(paramsT&&... params)
      {
        ROCKET_ASSERT_MSG(this->m_nelem < this->capacity(), "no space for new elements");

        size_t off = this->m_nelem;
        allocator_traits<allocator_type>::construct(*this, this->m_data + off, forward<paramsT>(params)...);
        this->m_nelem = static_cast<nelem_type>(off + 1U);
        return this->m_data[off];
      }

    void
    pop_back_unchecked()
      noexcept
      {
        ROCKET_ASSERT_MSG(this->m_nelem > 0, "no element to pop");

        size_t off = this->m_nelem - 1U;
        this->m_nelem = static_cast<nelem_type>(off);
        allocator_traits<allocator_type>::destroy(*this, this->m_data + off);
      }

    template<typename... paramsT>
    void
    append_n_unchecked(size_t total, const paramsT&... params)
      {
        for(size_t k = 0;  k != total;  ++k)
          this->emplace_back_unchecked(params...);
      }

    template<typename inputT>
    void
    append_range_unchecked(inputT first, inputT last)
      {
        for(auto it = move(first);  it != last;  ++it)
          this->emplace_back_unchecked(*it);
      }

    void
    pop_n_unchecked(size_t total)
      noexcept
      {
        for(size_t k = 0;  k != total;  ++k)
          this->pop_back_unchecked();
      }

    void
    copy_from(const storage_handle& other)
      {
        if(this->m_nelem < other.m_nelem) {
          // `*this` is shorter than `other`.
          size_type m = this->m_nelem;

          for(size_type k = 0;  k < m;  ++k)
            this->m_data[k] = other.m_data[k];

          for(size_type k = m;  k < other.m_nelem;  ++k)
            this->emplace_back_unchecked(other.m_data[k]);
        }
        else {
          // `other` is shorter than `*this`, or they have the same length.
          size_type m = other.m_nelem;

          for(size_type k = 0;  k < m;  ++k)
            this->m_data[k] = other.m_data[k];

          for(size_type k = this->m_nelem;  k > m;  --k)
            this->pop_back_unchecked();
        }
      }

    void
    move_from(storage_handle&& other)
      {
        if(this->m_nelem < other.m_nelem) {
          // `*this` is shorter than `other`.
          size_type m = this->m_nelem;

          for(size_type k = 0;  k < m;  ++k)
            this->m_data[k] = move(other.m_data[k]);

          for(size_type k = m;  k < other.m_nelem;  ++k)
            this->emplace_back_unchecked(move(other.m_data[k]));
        }
        else {
          // `other` is shorter than `*this`, or they have the same length.
          size_type m = other.m_nelem;

          for(size_type k = 0;  k < m;  ++k)
            this->m_data[k] = move(other.m_data[k]);

          for(size_type k = this->m_nelem;  k > m;  --k)
            this->pop_back_unchecked();
        }
      }

    void
    exchange_with(storage_handle& other)
      {
        if(this->m_nelem < other.m_nelem) {
          // `*this` is shorter than `other`.
          size_type m = this->m_nelem;

          for(size_type k = 0;  k < m;  ++k)
            noadl::xswap(this->m_data[k], other.m_data[k]);

          for(size_type k = m;  k < other.m_nelem;  ++k)
            this->emplace_back_unchecked(move(other.m_data[k]));

          for(size_type k = other.m_nelem;  k > m;  --k)
            other.pop_back_unchecked();
        }
        else {
          // `other` is shorter than `*this`, or they have the same length.
          size_type m = other.m_nelem;

          for(size_type k = 0;  k < m;  ++k)
            noadl::xswap(this->m_data[k], other.m_data[k]);

          for(size_type k = m;  k < this->m_nelem;  ++k)
            other.emplace_back_unchecked(move(this->m_data[k]));

          for(size_type k = this->m_nelem;  k > m;  --k)
            this->pop_back_unchecked();
        }
      }
  };

template<typename vectorT, typename valueT>
class iterator
  {
    friend vectorT;
    template<typename, typename> friend class iterator;

  public:
    using iterator_category  = ::std::random_access_iterator_tag;
    using value_type         = typename remove_cv<valueT>::type;
    using pointer            = valueT*;
    using reference          = valueT&;
    using difference_type    = ptrdiff_t;

  private:
    valueT* m_begin;
    valueT* m_cur;
    valueT* m_end;

  private:
    // This constructor is called by the container.
    constexpr
    iterator(valueT* begin, size_t ncur, size_t nend)
      noexcept
      :
        m_begin(begin), m_cur(begin + ncur), m_end(begin + nend)
      { }

  public:
    constexpr
    iterator()
      noexcept
      :
        m_begin(), m_cur(), m_end()
      { }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_convertible<yvalueT*, valueT*>::value)>
    constexpr
    iterator(const iterator<vectorT, yvalueT>& other)
      noexcept
      :
        m_begin(other.m_begin), m_cur(other.m_cur), m_end(other.m_end)
      { }

    template<typename yvalueT,
    ROCKET_ENABLE_IF(is_convertible<yvalueT*, valueT*>::value)>
    iterator&
    operator=(const iterator<vectorT, yvalueT>& other)
      & noexcept
      {
        this->m_begin = other.m_begin;
        this->m_cur = other.m_cur;
        this->m_end = other.m_end;
        return *this;
      }

  private:
    valueT*
    do_validate(valueT* cur, bool deref)
      const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        ROCKET_ASSERT_MSG((this->m_begin <= cur) && (cur <= this->m_end), "iterator out of range");
        ROCKET_ASSERT_MSG(!deref || (cur < this->m_end), "past-the-end iterator not dereferenceable");
        return cur;
      }

  public:
    reference
    operator*()
      const noexcept
      { return *(this->do_validate(this->m_cur, true));  }

    reference
    operator[](difference_type off)
      const noexcept
      { return *(this->do_validate(this->m_cur + off, true));  }

    pointer
    operator->()
      const noexcept
      { return ::std::addressof(**this);  }

    iterator&
    operator+=(difference_type off)
      & noexcept
      {
        this->m_cur = this->do_validate(this->m_cur + off, false);
        return *this;
      }

    iterator&
    operator-=(difference_type off)
      & noexcept
      {
        this->m_cur = this->do_validate(this->m_cur - off, false);
        return *this;
      }

    iterator
    operator+(difference_type off)
      const noexcept
      {
        auto res = *this;
        res += off;
        return res;
      }

    iterator
    operator-(difference_type off)
      const noexcept
      {
        auto res = *this;
        res -= off;
        return res;
      }

    iterator&
    operator++()
      noexcept
      { return *this += 1;  }

    iterator&
    operator--()
      noexcept
      { return *this -= 1;  }

    iterator
    operator++(int)
      noexcept
      { return noadl::exchange(*this, *this + 1);  }

    iterator
    operator--(int)
      noexcept
      { return noadl::exchange(*this, *this - 1);  }

    friend
    iterator
    operator+(difference_type off, const iterator& other)
      noexcept
      { return other + off;  }

    template<typename yvalueT>
    difference_type
    operator-(const iterator<vectorT, yvalueT>& other)
      const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        ROCKET_ASSERT_MSG(this->m_begin == other.m_begin, "iterator not compatible");
        ROCKET_ASSERT_MSG(this->m_end == other.m_end, "iterator not compatible");
        return this->m_cur - other.m_cur;
      }

    template<typename yvalueT>
    constexpr
    bool
    operator==(const iterator<vectorT, yvalueT>& other)
      const noexcept
      { return this->m_cur == other.m_cur;  }

    template<typename yvalueT>
    constexpr
    bool
    operator!=(const iterator<vectorT, yvalueT>& other)
      const noexcept
      { return this->m_cur != other.m_cur;  }

    template<typename yvalueT>
    bool
    operator<(const iterator<vectorT, yvalueT>& other)
      const noexcept
      { return *this - other < 0;  }

    template<typename yvalueT>
    bool
    operator>(const iterator<vectorT, yvalueT>& other)
      const noexcept
      { return *this - other > 0;  }

    template<typename yvalueT>
    bool
    operator<=(const iterator<vectorT, yvalueT>& other)
      const noexcept
      { return *this - other <= 0;  }

    template<typename yvalueT>
    bool
    operator>=(const iterator<vectorT, yvalueT>& other)
      const noexcept
      { return *this - other >= 0;  }
  };

}  // namespace details_static_vector
