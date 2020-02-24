// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_STATIC_VECTOR_HPP_
#  error Please include <rocket/static_vector.hpp> instead.
#endif

namespace details_static_vector {

template<typename allocT, size_t capacityT>
    class storage_handle : private allocator_wrapper_base_for<allocT>::type
  {
  public:
    using allocator_type   = allocT;
    using value_type       = typename allocator_type::value_type;
    using size_type        = typename allocator_traits<allocator_type>::size_type;

  private:
    using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;

  private:
    typename lowest_unsigned<capacityT - 1>::type m_nelem;
    union { value_type m_ebase[capacityT];  };

  public:
    explicit storage_handle(const allocator_type& alloc) noexcept
      :
        allocator_base(alloc),
        m_nelem(0)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_ebase, '*', sizeof(m_ebase));
#endif
      }
    explicit storage_handle(allocator_type&& alloc) noexcept
      :
        allocator_base(noadl::move(alloc)),
        m_nelem(0)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(this->m_ebase, '*', sizeof(m_ebase));
#endif
      }
    ~storage_handle()
      {
        auto ebase = this->m_ebase;
        auto nrem = this->m_nelem;
        ROCKET_ASSERT(nrem <= capacityT);
        while(nrem != 0) {
          --nrem;
          allocator_traits<allocator_type>::destroy(this->as_allocator(), ebase + nrem);
        }
#ifdef ROCKET_DEBUG
        this->m_nelem = static_cast<decltype(m_nelem)>(0xBAD1BEEF);
        ::std::memset(this->m_ebase, '~', sizeof(m_ebase));
#endif
      }

    storage_handle(const storage_handle&)
      = delete;
    storage_handle& operator=(const storage_handle&)
      = delete;

  public:
    const allocator_type& as_allocator() const noexcept
      {
        return static_cast<const allocator_base&>(*this);
      }
    allocator_type& as_allocator() noexcept
      {
        return static_cast<allocator_base&>(*this);
      }

    static constexpr size_type capacity() noexcept
      {
        return capacityT;
      }
    size_type max_size() const noexcept
      {
        return noadl::min(allocator_traits<allocator_type>::max_size(this->as_allocator()), this->capacity());
      }
    size_type check_size_add(size_type base, size_type add) const
      {
        auto nmax = this->max_size();
        ROCKET_ASSERT(base <= nmax);
        if(nmax - base < add) {
          noadl::sprintf_and_throw<length_error>("static_vector: max size exceeded (`%llu` + `%llu` > `%llu`)",
                                                 static_cast<unsigned long long>(base), static_cast<unsigned long long>(add),
                                                 static_cast<unsigned long long>(nmax));
        }
        return base + add;
      }
    const value_type* data() const noexcept
      {
        return this->m_ebase;
      }
    value_type* mut_data() noexcept
      {
        return this->m_ebase;
      }
    bool empty() const noexcept
      {
        return this->m_nelem == 0;
      }
    size_type size() const noexcept
      {
        return this->m_nelem;
      }

    operator const storage_handle* () const noexcept
      {
        return this;
      }
    operator storage_handle* () noexcept
      {
        return this;
      }

    template<typename... paramsT> value_type* emplace_back_unchecked(paramsT&&... params)
      {
        ROCKET_ASSERT(this->size() < this->capacity());
        auto ebase = this->m_ebase;
        size_t nelem = this->m_nelem;
        allocator_traits<allocator_type>::construct(this->as_allocator(), ebase + nelem, noadl::forward<paramsT>(params)...);
        this->m_nelem = static_cast<decltype(m_nelem)>(++nelem);
        return ebase + nelem - 1;
      }
    void pop_back_n_unchecked(size_type n) noexcept
      {
        ROCKET_ASSERT(n <= this->size());
        if(n == 0) {
          return;
        }
        auto ebase = this->m_ebase;
        size_t nelem = this->m_nelem;
        for(size_type i = n; i != 0; --i) {
          this->m_nelem = static_cast<decltype(m_nelem)>(--nelem);
          allocator_traits<allocator_type>::destroy(this->as_allocator(), ebase + nelem);
        }
      }
  };

template<typename vectorT, typename valueT> class vector_iterator
  {
    template<typename, typename> friend class vector_iterator;
    friend vectorT;

  public:
    using iterator_category  = random_access_iterator_tag;
    using value_type         = typename remove_cv<valueT>::type;
    using pointer            = valueT*;
    using reference          = valueT&;
    using difference_type    = ptrdiff_t;

    using parent_type   = storage_handle<typename vectorT::allocator_type, vectorT::capacity()>;

  private:
    const parent_type* m_ref;
    pointer m_ptr;

  private:
    constexpr vector_iterator(const parent_type* ref, pointer ptr) noexcept
      :
        m_ref(ref), m_ptr(ptr)
      {
      }

  public:
    constexpr vector_iterator() noexcept
      :
        vector_iterator(nullptr, nullptr)
      {
      }
    template<typename yvalueT, ROCKET_ENABLE_IF(is_convertible<yvalueT*, valueT*>::value)>
            constexpr vector_iterator(const vector_iterator<vectorT, yvalueT>& other) noexcept
      :
        vector_iterator(other.m_ref, other.m_ptr)
      {
      }

  private:
    pointer do_assert_valid_pointer(pointer ptr, bool deref) const noexcept
      {
        auto ref = this->m_ref;
        ROCKET_ASSERT_MSG(ref, "iterator not initialized");
        auto dist = static_cast<size_t>(ptr - ref->data());
        ROCKET_ASSERT_MSG(dist <= ref->size(), "iterator invalidated");
        ROCKET_ASSERT_MSG(!deref || (dist < ref->size()), "past-the-end iterator not dereferenceable");
        return ptr;
      }

  public:
    const parent_type* parent() const noexcept
      {
        return this->m_ref;
      }

    pointer tell() const noexcept
      {
        auto ptr = this->do_assert_valid_pointer(this->m_ptr, false);
        return ptr;
      }
    pointer tell_owned_by(const parent_type* ref) const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_ref == ref, "iterator not belonging to the same container");
        return this->tell();
      }
    vector_iterator& seek(pointer ptr) noexcept
      {
        this->m_ptr = this->do_assert_valid_pointer(ptr, false);
        return *this;
      }

    reference operator*() const noexcept
      {
        auto ptr = this->do_assert_valid_pointer(this->m_ptr, true);
        return *ptr;
      }
    pointer operator->() const noexcept
      {
        auto ptr = this->do_assert_valid_pointer(this->m_ptr, true);
        return ptr;
      }
    reference operator[](difference_type off) const noexcept
      {
        auto ptr = this->do_assert_valid_pointer(this->m_ptr + off, true);
        return *ptr;
      }
  };

template<typename vectorT, typename valueT>
    vector_iterator<vectorT, valueT>& operator++(vector_iterator<vectorT, valueT>& rhs) noexcept
  {
    return rhs.seek(rhs.tell() + 1);
  }
template<typename vectorT, typename valueT>
    vector_iterator<vectorT, valueT>& operator--(vector_iterator<vectorT, valueT>& rhs) noexcept
  {
    return rhs.seek(rhs.tell() - 1);
  }

template<typename vectorT, typename valueT>
    vector_iterator<vectorT, valueT> operator++(vector_iterator<vectorT, valueT>& lhs, int) noexcept
  {
    auto res = lhs;
    lhs.seek(lhs.tell() + 1);
    return res;
  }
template<typename vectorT, typename valueT>
    vector_iterator<vectorT, valueT> operator--(vector_iterator<vectorT, valueT>& lhs, int) noexcept
  {
    auto res = lhs;
    lhs.seek(lhs.tell() - 1);
    return res;
  }

template<typename vectorT, typename valueT>
    vector_iterator<vectorT, valueT>& operator+=(vector_iterator<vectorT, valueT>& lhs,
                                                 typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
  {
    return lhs.seek(lhs.tell() + rhs);
  }
template<typename vectorT, typename valueT>
    vector_iterator<vectorT, valueT>& operator-=(vector_iterator<vectorT, valueT>& lhs,
                                                 typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
  {
    return lhs.seek(lhs.tell() - rhs);
  }

template<typename vectorT, typename valueT>
    vector_iterator<vectorT, valueT> operator+(const vector_iterator<vectorT, valueT>& lhs,
                                               typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
  {
    auto res = lhs;
    res.seek(res.tell() + rhs);
    return res;
  }
template<typename vectorT, typename valueT>
    vector_iterator<vectorT, valueT> operator-(const vector_iterator<vectorT, valueT>& lhs,
                                               typename vector_iterator<vectorT, valueT>::difference_type rhs) noexcept
  {
    auto res = lhs;
    res.seek(res.tell() - rhs);
    return res;
  }

template<typename vectorT, typename valueT>
    vector_iterator<vectorT, valueT> operator+(typename vector_iterator<vectorT, valueT>::difference_type lhs,
                                               const vector_iterator<vectorT, valueT>& rhs) noexcept
  {
    auto res = rhs;
    res.seek(res.tell() + lhs);
    return res;
  }
template<typename vectorT, typename xvalueT, typename yvalueT>
    typename vector_iterator<vectorT, xvalueT>::difference_type operator-(const vector_iterator<vectorT, xvalueT>& lhs,
                                                                          const vector_iterator<vectorT, yvalueT>& rhs) noexcept
  {
    return lhs.tell_owned_by(rhs.parent()) - rhs.tell();
  }

template<typename vectorT, typename xvalueT, typename yvalueT>
    bool operator==(const vector_iterator<vectorT, xvalueT>& lhs,
                    const vector_iterator<vectorT, yvalueT>& rhs) noexcept
  {
    return lhs.tell() == rhs.tell();
  }
template<typename vectorT, typename xvalueT, typename yvalueT>
    bool operator!=(const vector_iterator<vectorT, xvalueT>& lhs,
                    const vector_iterator<vectorT, yvalueT>& rhs) noexcept
  {
    return lhs.tell() != rhs.tell();
  }

template<typename vectorT, typename xvalueT, typename yvalueT>
    bool operator<(const vector_iterator<vectorT, xvalueT>& lhs,
                   const vector_iterator<vectorT, yvalueT>& rhs) noexcept
  {
    return lhs.tell_owned_by(rhs.parent()) < rhs.tell();
  }
template<typename vectorT, typename xvalueT, typename yvalueT>
    bool operator>(const vector_iterator<vectorT, xvalueT>& lhs,
                   const vector_iterator<vectorT, yvalueT>& rhs) noexcept
  {
    return lhs.tell_owned_by(rhs.parent()) > rhs.tell();
  }
template<typename vectorT, typename xvalueT, typename yvalueT>
    bool operator<=(const vector_iterator<vectorT, xvalueT>& lhs,
                    const vector_iterator<vectorT, yvalueT>& rhs) noexcept
  {
    return lhs.tell_owned_by(rhs.parent()) <= rhs.tell();
  }
template<typename vectorT, typename xvalueT, typename yvalueT>
    bool operator>=(const vector_iterator<vectorT, xvalueT>& lhs,
                    const vector_iterator<vectorT, yvalueT>& rhs) noexcept
  {
    return lhs.tell_owned_by(rhs.parent()) >= rhs.tell();
  }

// Insertion helpers.
struct append_tag
  {
  }
constexpr append;

template<typename vectorT, typename... paramsT> void tagged_append(vectorT* vec, append_tag, paramsT&&... params)
  {
    vec->append(noadl::forward<paramsT>(params)...);
  }

struct emplace_back_tag
  {
  }
constexpr emplace_back;

template<typename vectorT, typename... paramsT> void tagged_append(vectorT* vec, emplace_back_tag, paramsT&&... params)
  {
    vec->emplace_back(noadl::forward<paramsT>(params)...);
  }

struct push_back_tag
  {
  }
constexpr push_back;

template<typename vectorT, typename... paramsT> void tagged_append(vectorT* vec, push_back_tag, paramsT&&... params)
  {
    vec->push_back(noadl::forward<paramsT>(params)...);
  }

}  // namespace details_static_vector
