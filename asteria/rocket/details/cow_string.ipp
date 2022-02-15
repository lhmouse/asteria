// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRING_HPP_
#  error Please include <rocket/cow_string.hpp> instead.
#endif

namespace details_cow_string {

struct storage_header
  {
    mutable reference_counter<long> nref;

    explicit
    storage_header() noexcept
      : nref()
      { }
  };

template<typename allocT>
struct basic_storage
  : public storage_header,
    public allocator_wrapper_base_for<allocT>::type
  {
    using allocator_type   = allocT;
    using value_type       = typename allocator_type::value_type;
    using size_type        = typename allocator_traits<allocator_type>::size_type;

    static constexpr size_type
    min_nblk_for_nchar(size_t nchar) noexcept
      {
        // Note this is correct even when `nchar` is zero, as long as
        // `basic_storage` is larger than `value_type`. The `data` field
        // counts as the null terminator.
        return (nchar * sizeof(value_type) + sizeof(basic_storage) - 1)
                / sizeof(basic_storage) + 1;
      }

    static constexpr size_t
    max_nchar_for_nblk(size_type nblk) noexcept
      { return (nblk - 1) * sizeof(basic_storage) / sizeof(value_type);  }

    size_type nblk;
    union { value_type data[1];  };

    basic_storage(const allocator_type& xalloc, size_type xnblk) noexcept
      : allocator_wrapper_base_for<allocT>::type(xalloc),
        nblk(xnblk)
      {
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this->data), '*',
                 sizeof(value_type) + (this->nblk - 1) * sizeof(basic_storage));
#endif
      }

    ~basic_storage()
      {
#ifdef ROCKET_DEBUG
        ::std::memset(static_cast<void*>(this->data), '~',
                 sizeof(value_type) + (this->nblk - 1) * sizeof(basic_storage));
#endif
      }

    basic_storage(const basic_storage&)
      = delete;

    basic_storage&
    operator=(const basic_storage&)
      = delete;
  };

template<typename allocT, typename traitsT>
class storage_handle
  : private allocator_wrapper_base_for<allocT>::type
  {
  public:
    using allocator_type   = allocT;
    using traits_type      = traitsT;
    using value_type       = typename allocator_type::value_type;
    using size_type        = typename allocator_traits<allocator_type>::size_type;

    static constexpr value_type null_char[1] = { };

  private:
    using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;
    using storage           = basic_storage<allocator_type>;
    using storage_allocator = typename allocator_traits<allocator_type>::template rebind_alloc<storage>;
    using storage_pointer   = typename allocator_traits<storage_allocator>::pointer;

  private:
    storage_pointer m_qstor = nullptr;

  public:
    explicit constexpr
    storage_handle(const allocator_type& alloc) noexcept
      : allocator_base(alloc)
      { }

    explicit constexpr
    storage_handle(allocator_type&& alloc) noexcept
      : allocator_base(::std::move(alloc))
      { }

    ~storage_handle()
      { this->deallocate();  }

    storage_handle(const storage_handle&)
      = delete;

    storage_handle&
    operator=(const storage_handle&)
      = delete;

  private:
    void
    do_reset(storage_pointer qstor_new) noexcept
      {
        // Decrement the reference count with acquire-release semantics to prevent
        // races on `*qstor`.
        auto qstor = ::std::exchange(this->m_qstor, qstor_new);
        if(ROCKET_EXPECT(!qstor))
          return;

        if(ROCKET_EXPECT(qstor->nref.decrement() != 0))
          return;

        // Unlike vectors, strings require value types to be complete.
        // This is a direct call without type erasure.
        this->do_destroy_storage(qstor);
      }

    ROCKET_NEVER_INLINE static void
    do_destroy_storage(storage_pointer qstor) noexcept
      {
        auto nblk = qstor->nblk;
        storage_allocator st_alloc(*qstor);
        noadl::destroy(noadl::unfancy(qstor));
        allocator_traits<storage_allocator>::deallocate(st_alloc, qstor, nblk);
      }

  public:
    constexpr const allocator_type&
    as_allocator() const noexcept
      { return static_cast<const allocator_base&>(*this);  }

    allocator_type&
    as_allocator() noexcept
      { return static_cast<allocator_base&>(*this);  }

    ROCKET_PURE bool
    unique() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return false;
        return qstor->nref.unique();
      }

    long
    use_count() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return qstor->nref.get();
      }

    ROCKET_PURE size_type
    capacity() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return storage::max_nchar_for_nblk(qstor->nblk);
      }

    size_type
    max_size() const noexcept
      {
        storage_allocator st_alloc(this->as_allocator());
        auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
        return storage::max_nchar_for_nblk(max_nblk / 2);
      }

    size_type
    check_size_add(size_type base, size_type add) const
      {
        auto nmax = this->max_size();
        ROCKET_ASSERT(base <= nmax);
        if(nmax - base < add) {
          noadl::sprintf_and_throw<length_error>(
                "cow_string: max size exceeded (`%lld` + `%lld` > `%lld`)",
                static_cast<long long>(base), static_cast<long long>(add),
                static_cast<long long>(nmax));
        }
        return base + add;
      }

    size_type
    round_up_capacity(size_type res_arg) const
      {
        size_type cap = this->check_size_add(0, res_arg);
        auto nblk = storage::min_nblk_for_nchar(cap);
        return storage::max_nchar_for_nblk(nblk);
      }

    ROCKET_PURE const value_type*
    data() const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return null_char;
        return qstor->data;
      }

    value_type*
    mut_data_opt() noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor || !qstor->nref.unique())
          return nullptr;
        return qstor->data;
      }

    ROCKET_NEVER_INLINE value_type*
    reallocate_more(const value_type* src, size_type len, size_type add)
      {
        // Calculate the combined length of string (len + add).
        // The first part is copied from `src`. The second part is left uninitialized.
        size_type cap = this->check_size_add(len, add);

        // Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
        auto nblk = storage::min_nblk_for_nchar(cap);
        storage_allocator st_alloc(this->as_allocator());
        auto qstor = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
        noadl::construct(noadl::unfancy(qstor), this->as_allocator(), nblk);

        // Add a null character anyway.
        // The user still has to keep track of it if the storage is not fully utilized.
        traits_type::assign(qstor->data[cap], value_type());

        // Copy characters into the new block if any.
        // This shall not throw exceptions.
        if(len)
          traits_type::copy(qstor->data, src, len);

        traits_type::assign(qstor->data[len], value_type());

        // Set up the new storage.
        this->do_reset(qstor);
        return qstor->data;
      }

    void
    deallocate() noexcept
      { this->do_reset(nullptr);  }

    void
    share_with(const storage_handle& other) noexcept
      {
        auto qstor = other.m_qstor;
        if(qstor)
          qstor->nref.increment();
        this->do_reset(qstor);
      }

    void
    exchange_with(storage_handle& other) noexcept
      { ::std::swap(this->m_qstor, other.m_qstor);  }
  };

template<typename allocT, typename traitsT>
constexpr typename allocT::value_type storage_handle<allocT, traitsT>::null_char[1];

// Implement relational operators.
template<typename charT, typename traitsT>
struct comparator
  {
    using char_type    = charT;
    using traits_type  = traitsT;
    using size_type    = size_t;

    static constexpr int
    inequality(const char_type* s1, size_type n1, const char_type* s2, size_type n2) noexcept
      {
        return (n1 != n2) ? 2  // length not equal
             : (s1 == s2) ? 0  // reflexive
                          : traits_type::compare(s1, s2, n1);
      }

    static constexpr int
    relation(const char_type* s1, size_type n1, const char_type* s2, size_type n2) noexcept
      {
        return (n1 == n2) ?      traits_type::compare(s1, s2, n1)
             : (n1 >= n2) ? 1 |  traits_type::compare(s1, s2, n2)
                          : 1 | ~traits_type::compare(s2, s1, n1);
      }
  };

// Implement the FNV-1a hash algorithm.
template<typename charT, typename traitsT>
class basic_hasher
  {
  private:
    static constexpr char32_t xoffset = 0x811C9DC5;
    static constexpr char32_t xprime = 0x1000193;
    char32_t m_reg = xoffset;

  public:
    constexpr basic_hasher&
    append(charT c) noexcept
      {
        char32_t word = static_cast<char32_t>(c);
        for(unsigned k = 0;  k < sizeof(charT);  ++k)
          this->m_reg = (this->m_reg ^ ((word >> k * 8) & 0xFF)) * xprime;
        return *this;
      }

    constexpr basic_hasher&
    append(const charT* s, size_t n)
      {
        const charT* sp = s;
        while(sp != s + n)
          this->append(*(sp++));
        return *this;
      }

    constexpr basic_hasher&
    append(const charT* s)
      {
        const charT* sp = s;
        while(!traitsT::eq(*sp, charT()))
          this->append(*(sp++));
        return *this;
      }

    constexpr size_t
    finish() noexcept
      {
        char32_t reg = this->m_reg;
        this->m_reg = xoffset;
        return reg;
      }
  };

template<typename stringT, typename charT>
class string_iterator
  {
    template<typename, typename>
    friend class string_iterator;

    friend stringT;

  public:
    using iterator_category  = ::std::random_access_iterator_tag;
    using value_type         = typename remove_cv<charT>::type;
    using pointer            = charT*;
    using reference          = charT&;
    using difference_type    = ptrdiff_t;

  private:
    charT* m_begin;
    charT* m_cur;
    charT* m_end;

  private:
    // This constructor is called by the container.
    constexpr
    string_iterator(charT* begin, size_t ncur, size_t nend) noexcept
      : m_begin(begin), m_cur(begin + ncur), m_end(begin + nend)
      { }

  public:
    constexpr
    string_iterator() noexcept
      : m_begin(), m_cur(), m_end()
      { }

    template<typename ycharT,
    ROCKET_ENABLE_IF(is_convertible<ycharT*, charT*>::value)>
    constexpr
    string_iterator(const string_iterator<stringT, ycharT>& other) noexcept
      : m_begin(other.m_begin),
        m_cur(other.m_cur),
        m_end(other.m_end)
      { }

    template<typename ycharT,
    ROCKET_ENABLE_IF(is_convertible<ycharT*, charT*>::value)>
    string_iterator&
    operator=(const string_iterator<stringT, ycharT>& other) noexcept
      { this->m_begin = other.m_begin;
        this->m_cur = other.m_cur;
        this->m_end = other.m_end;
        return *this;  }

  private:
    charT*
    do_validate(charT* cur, bool deref) const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        ROCKET_ASSERT_MSG((this->m_begin <= cur) && (cur <= this->m_end), "iterator out of range");
        ROCKET_ASSERT_MSG(!deref || (cur < this->m_end), "past-the-end iterator not dereferenceable");
        return cur;
      }

  public:
    reference
    operator*() const noexcept
      { return *(this->do_validate(this->m_cur, true));  }

    reference
    operator[](difference_type off) const noexcept
      { return *(this->do_validate(this->m_cur + off, true));  }

    pointer
    operator->() const noexcept
      { return ::std::addressof(**this);  }

    string_iterator&
    operator+=(difference_type off) noexcept
      {
        this->m_cur = this->do_validate(this->m_cur + off, false);
        return *this;
      }

    string_iterator&
    operator-=(difference_type off) noexcept
      {
        this->m_cur = this->do_validate(this->m_cur - off, false);
        return *this;
      }

    string_iterator
    operator+(difference_type off) const noexcept
      {
        auto res = *this;
        res += off;
        return res;
      }

    string_iterator
    operator-(difference_type off) const noexcept
      {
        auto res = *this;
        res -= off;
        return res;
      }

    string_iterator&
    operator++() noexcept
      { return *this += 1;  }

    string_iterator&
    operator--() noexcept
      { return *this -= 1;  }

    string_iterator
    operator++(int) noexcept
      { return ::std::exchange(*this, *this + 1);  }

    string_iterator
    operator--(int) noexcept
      { return ::std::exchange(*this, *this - 1);  }

    friend
    string_iterator
    operator+(difference_type off, const string_iterator& other) noexcept
      { return other + off;  }

    template<typename ycharT>
    difference_type
    operator-(const string_iterator<stringT, ycharT>& other) const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "iterator not initialized");
        ROCKET_ASSERT_MSG(this->m_begin == other.m_begin, "iterator not compatible");
        ROCKET_ASSERT_MSG(this->m_end == other.m_end, "iterator not compatible");
        return this->m_cur - other.m_cur;
      }

    template<typename ycharT>
    constexpr bool
    operator==(const string_iterator<stringT, ycharT>& other) const noexcept
      { return this->m_cur == other.m_cur;  }

    template<typename ycharT>
    constexpr bool
    operator!=(const string_iterator<stringT, ycharT>& other) const noexcept
      { return this->m_cur != other.m_cur;  }

    template<typename ycharT>
    bool
    operator<(const string_iterator<stringT, ycharT>& other) const noexcept
      { return *this - other < 0;  }

    template<typename ycharT>
    bool
    operator>(const string_iterator<stringT, ycharT>& other) const noexcept
      { return *this - other > 0;  }

    template<typename ycharT>
    bool
    operator<=(const string_iterator<stringT, ycharT>& other) const noexcept
      { return *this - other <= 0;  }

    template<typename ycharT>
    bool
    operator>=(const string_iterator<stringT, ycharT>& other) const noexcept
      { return *this - other >= 0;  }
  };

}  // namespace details_cow_string
