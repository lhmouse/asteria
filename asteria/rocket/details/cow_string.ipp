// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRING_HPP_
#  error Please include <rocket/cow_string.hpp> instead.
#endif

namespace details_cow_string {

struct storage_header
  {
    mutable reference_counter<long> nref;

    explicit
    storage_header()
    noexcept
      : nref()
      { }
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
    struct storage : storage_header
      {
        allocator_type alloc;
        size_type nblk;
        value_type data[0];

        storage(const allocator_type& xalloc, size_type xnblk)
        noexcept
          : alloc(xalloc), nblk(xnblk)
          {
#ifdef ROCKET_DEBUG
            ::std::memset(static_cast<void*>(this->data), '*', sizeof(storage) * (this->nblk - 1));
#endif
          }

        ~storage()
          {
#ifdef ROCKET_DEBUG
            ::std::memset(static_cast<void*>(this->data), '~', sizeof(storage) * (this->nblk - 1));
#endif
          }

        storage(const storage&)
          = delete;

        storage&
        operator=(const storage&)
          = delete;
      };

    static constexpr
    size_type
    min_nblk_for_nchar(size_type nchar)
    noexcept
      { return (sizeof(value_type) * (nchar + 1) + sizeof(storage) - 1) / sizeof(storage) + 1;  }

    static constexpr
    size_type
    max_nchar_for_nblk(size_type nblk)
    noexcept
      { return sizeof(storage) * (nblk - 1) / sizeof(value_type) - 1;  }

    using allocator_base    = typename allocator_wrapper_base_for<allocator_type>::type;
    using storage_allocator = typename allocator_traits<allocator_type>::template rebind_alloc<storage>;
    using storage_pointer   = typename allocator_traits<storage_allocator>::pointer;

  private:
    storage_pointer m_qstor;

  public:
    explicit constexpr
    storage_handle(const allocator_type& alloc)
    noexcept
      : allocator_base(alloc), m_qstor()
      { }

    explicit constexpr
    storage_handle(allocator_type&& alloc)
    noexcept
      : allocator_base(::std::move(alloc)), m_qstor()
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
    do_reset(storage_pointer qstor_new)
    noexcept
      {
        // Decrement the reference count with acquire-release semantics to prevent races
        // on `qstor->alloc`.
        auto qstor = ::std::exchange(this->m_qstor, qstor_new);
        if(ROCKET_EXPECT(!qstor))
          return;

        if(ROCKET_EXPECT(!qstor->nref.decrement()))
          return;

        // Unlike vectors, strings require value types to be complete.
        // This is a direct call without type erasure.
        this->do_destroy_storage(qstor);
      }

    ROCKET_NOINLINE static
    void
    do_destroy_storage(storage_pointer qstor)
    noexcept
      {
        auto nblk = qstor->nblk;
        storage_allocator st_alloc(qstor->alloc);
        noadl::destroy_at(noadl::unfancy(qstor));
        allocator_traits<storage_allocator>::deallocate(st_alloc, qstor, nblk);
      }

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

    ROCKET_PURE_FUNCTION
    bool
    unique()
    const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return false;
        return qstor->nref.unique();
      }

    long
    use_count()
    const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return qstor->nref.get();
      }

    ROCKET_PURE_FUNCTION
    size_type
    capacity()
    const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return 0;
        return this->max_nchar_for_nblk(qstor->nblk);
      }

    size_type
    max_size()
    const noexcept
      {
        storage_allocator st_alloc(this->as_allocator());
        auto max_nblk = allocator_traits<storage_allocator>::max_size(st_alloc);
        return this->max_nchar_for_nblk(max_nblk / 2);
      }

    size_type
    check_size_add(size_type base, size_type add)
    const
      {
        auto nmax = this->max_size();
        ROCKET_ASSERT(base <= nmax);
        if(nmax - base < add)
          noadl::sprintf_and_throw<length_error>("cow_string: max size exceeded (`%lld` + `%lld` > `%lld`)",
                                                 static_cast<long long>(base), static_cast<long long>(add),
                                                 static_cast<long long>(nmax));
        return base + add;
      }

    size_type
    round_up_capacity(size_type res_arg)
    const
      {
        auto cap = this->check_size_add(0, res_arg);
        auto nblk = this->min_nblk_for_nchar(cap);
        return this->max_nchar_for_nblk(nblk);
      }

    ROCKET_PURE_FUNCTION
    const value_type*
    data()
    const noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor)
          return null_char;
        return qstor->data;
      }

    value_type*
    mut_data_opt()
    noexcept
      {
        auto qstor = this->m_qstor;
        if(!qstor || !qstor->nref.unique())
          return nullptr;
        return qstor->data;
      }

    ROCKET_NOINLINE
    value_type*
    reallocate_more(const value_type* src, size_type len, size_type add)
      {
        // Calculate the combined length of string (len + add).
        // The first part is copied from `src`. The second part is left uninitialized.
        auto cap = this->check_size_add(len, add);

        // Allocate an array of `storage` large enough for a header + `cap` instances of `value_type`.
        auto nblk = this->min_nblk_for_nchar(cap);
        storage_allocator st_alloc(this->as_allocator());
        auto qstor = allocator_traits<storage_allocator>::allocate(st_alloc, nblk);
        noadl::construct_at(noadl::unfancy(qstor), this->as_allocator(), nblk);

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
        return qstor->data + len;
      }

    void
    deallocate()
    noexcept
      { this->do_reset(nullptr);  }

    void
    share_with(const storage_handle& other)
    noexcept
      {
        auto qstor = other.m_qstor;
        if(qstor)
          qstor->nref.increment();
        this->do_reset(qstor);
      }

    void
    exchange_with(storage_handle& other)
    noexcept
      { ::std::swap(this->m_qstor, other.m_qstor);  }

    constexpr operator
    const storage_handle*()
    const noexcept
      { return this;  }

    operator
    storage_handle*()
    noexcept
      { return this;  }
  };

template<typename allocT, typename traitsT>
constexpr typename allocT::value_type storage_handle<allocT, traitsT>::null_char[1];

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
    pointer m_begin;
    pointer m_cur;
    pointer m_end;

  private:
    // This constructor is called by the container.
    constexpr
    string_iterator(charT* begin, size_t ncur, size_t nend)
    noexcept
      : m_begin(begin), m_cur(begin + ncur), m_end(begin + nend)
      { }

  public:
    constexpr
    string_iterator()
    noexcept
      : m_begin(), m_cur(), m_end()
      { }

    template<typename ycharT,
    ROCKET_ENABLE_IF(is_convertible<ycharT*, charT*>::value)>
    constexpr
    string_iterator(const string_iterator<stringT, ycharT>& other)
    noexcept
      : m_begin(other.m_begin), m_cur(other.m_cur), m_end(other.m_end)
      { }

  private:
    pointer
    do_validate(pointer cur, bool deref)
    const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "Iterator not initialized");
        ROCKET_ASSERT_MSG((this->m_begin <= cur) && (cur <= this->m_end), "Iterator out of range");
        ROCKET_ASSERT_MSG(!deref || (cur < this->m_end), "Past-the-end iterator not dereferenceable");
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
      { return this->do_validate(this->m_cur, true);  }

    string_iterator&
    operator+=(difference_type off)
    noexcept
      {
        this->m_cur = this->do_validate(this->m_cur + off, false);
        return *this;
      }

    string_iterator&
    operator++()
    noexcept
      { return *this += difference_type(1);  }

    string_iterator
    operator+(difference_type off)
    const noexcept
      {
        auto res = *this;
        res += off;
        return res;
      }

    string_iterator&
    operator-=(difference_type off)
    noexcept
      {
        this->m_cur = this->do_validate(this->m_cur - off, false);
        return *this;
      }

    string_iterator&
    operator--()
    noexcept
      { return *this -= difference_type(1);  }

    string_iterator
    operator-(difference_type off)
    const noexcept
      {
        auto res = *this;
        res -= off;
        return res;
      }

    friend
    string_iterator
    operator+(difference_type off, const string_iterator& other)
    noexcept
      { return other + off;  }

    template<typename ycharT>
    difference_type
    operator-(const string_iterator<stringT, ycharT>& other)
    const noexcept
      {
        ROCKET_ASSERT_MSG(this->m_begin, "Iterator not initialized");
        ROCKET_ASSERT_MSG(this->m_begin == other.m_begin, "Iterator not compatible");
        return this->m_cur - other.m_cur;
      }
  };

template<typename stringT, typename xcharT, typename ycharT>
bool
operator==(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs - rhs == 0;  }

template<typename stringT, typename xcharT, typename ycharT>
bool
operator!=(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs - rhs != 0;  }

template<typename stringT, typename xcharT, typename ycharT>
bool
operator<(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs - rhs < 0;  }

template<typename stringT, typename xcharT, typename ycharT>
bool
operator>(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs - rhs > 0;  }

template<typename stringT, typename xcharT, typename ycharT>
bool
operator<=(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs - rhs <= 0;  }

template<typename stringT, typename xcharT, typename ycharT>
bool
operator>=(const string_iterator<stringT, xcharT>& lhs, const string_iterator<stringT, ycharT>& rhs)
noexcept
  { return lhs - rhs >= 0;  }

// Implement relational operators.
template<typename charT, typename traitsT>
struct comparator
  {
    using char_type    = charT;
    using traits_type  = traitsT;
    using size_type    = size_t;

    static
    int
    inequality(const char_type* s1, size_type n1, const char_type* s2, size_type n2)
    noexcept
      {
        if(n1 != n2)
          return 2;
        else if(s1 == s2)
          return 0;
        else
          return traits_type::compare(s1, s2, n1);
      }

    static
    int
    relation(const char_type* s1, size_type n1, const char_type* s2, size_type n2)
    noexcept
      {
        if(n1 < n2)
          return (traits_type::compare(s1, s2, n1) > 0) ? +1 : -1;
        else if(n1 > n2)
          return (traits_type::compare(s1, s2, n1) < 0) ? -1 : +1;
        else
          return traits_type::compare(s1, s2, n1);
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
    constexpr
    basic_hasher&
    append(const charT& c)
    noexcept
      {
        char32_t word = static_cast<char32_t>(c);
        char32_t reg = this->m_reg;

        for(size_t k = 0;  k < sizeof(c);  ++k)
          reg = (reg ^ ((word >> k * 8) & 0xFF)) * xprime;

        this->m_reg = reg;
        return *this;
      }

    constexpr
    basic_hasher&
    append(const charT* s, size_t n)
      {
        for(auto sp = s;  sp != s + n;  ++sp)
          this->append(*sp);
        return *this;
      }

    constexpr
    basic_hasher&
    append(const charT* s)
      {
        for(auto sp = s;  !traitsT::eq(*sp, charT());  ++sp)
          this->append(*sp);
        return *this;
      }

    constexpr
    size_t
    finish()
    noexcept
      {
        char32_t reg = this->m_reg;
        this->m_reg = xoffset;
        return reg;
      }
  };

}  // namespace details_cow_string
